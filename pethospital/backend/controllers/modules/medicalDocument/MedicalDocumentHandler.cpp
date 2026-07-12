#include "MedicalDocumentHandler.h"

#include "../../../controllers/auth/encrypt/encrypt.h"
#include "../../../services/reporting/ReportRenderer.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
class TransactionGuard
{
public:
    explicit TransactionGuard(mysqlx::Session &session) : session_(session) { session_.startTransaction(); }
    ~TransactionGuard() { if (!committed_) rollbackTransactionQuietly(session_); }
    void commit() { session_.commit(); committed_ = true; }

private:
    mysqlx::Session &session_;
    bool committed_{false};
};

std::string bodyString(const nlohmann::json &body, const char *key)
{
    if (!body.contains(key) || body[key].is_null()) return "";
    return body[key].is_string() ? body[key].get<std::string>() : "";
}

bool textFieldsAreSafe(const nlohmann::json &body)
{
    const char *keys[] = {
        "chiefComplaint", "presentIllness", "pastHistory", "allergies",
        "physicalExam", "diagnosis", "treatmentPlan", "dischargeAdvice",
    };
    for (const char *key : keys)
    {
        if (body.contains(key) && (!body[key].is_string() || body[key].get<std::string>().size() > 20000)) return false;
    }
    return true;
}

bool structuredDataIsSafe(const nlohmann::json &body)
{
    if (!body.contains("structuredData")) return true;
    return body["structuredData"].is_object() && body["structuredData"].dump().size() <= 100 * 1024;
}

bool prescriptionInstructionsAreSafe(const nlohmann::json &items)
{
    if (!items.is_array() || items.size() > 100) return false;
    for (const auto &item : items)
    {
        if (!item.is_object() || !item.contains("id") || !item["id"].is_number_unsigned()) return false;
        const std::pair<const char *, std::size_t> fields[] = {
            {"dosage", 128}, {"frequency", 128}, {"route", 128}, {"instructions", 500},
        };
        for (const auto &[key, maximum] : fields)
        {
            if (item.contains(key) && (!item[key].is_string() || item[key].get<std::string>().size() > maximum)) return false;
        }
        const int durationDays = item.value("durationDays", 0);
        if (durationDays < 0 || durationDays > 3650) return false;
    }
    return true;
}

nlohmann::json parseJsonColumn(const mysqlx::Value &value)
{
    if (value.isNull()) return nlohmann::json::object();
    try
    {
        const std::string text = value.get<std::string>();
        return text.empty() ? nlohmann::json::object() : nlohmann::json::parse(text);
    }
    catch (...)
    {
        return nlohmann::json::object();
    }
}

void updatePrescriptionInstructions(
    mysqlx::Session &session,
    unsigned long long documentId,
    const nlohmann::json &items)
{
    if (!items.is_array()) return;
    for (const auto &item : items)
    {
        if (!item.is_object()) continue;
        const unsigned long long itemId = item.value("id", 0ULL);
        const int durationDays = item.value("durationDays", 0);
        if (itemId == 0 || durationDays < 0) continue;
        session.sql("UPDATE medical_prescription_items SET dosage=?, frequency=?, route=?, duration_days=?, instructions=? "
                    "WHERE id=? AND medical_document_id=?")
            .bind(item.value("dosage", ""), item.value("frequency", ""), item.value("route", ""),
                  durationDays, item.value("instructions", ""), itemId, documentId)
            .execute();
    }
}

nlohmann::json artifactJson(const mysqlx::Row &row)
{
    return {
        {"id", row[0].get<unsigned long long>()},
        {"format", row[1].get<std::string>()},
        {"sha256", row[2].get<std::string>()},
        {"byteSize", row[3].get<unsigned long long>()},
        {"generatedAt", row[4].get<std::string>()},
    };
}

nlohmann::json loadLatestSnapshot(mysqlx::Session &session, unsigned long long documentId)
{
    auto result = session.sql(
        "SELECT CAST(snapshot_json AS CHAR) FROM medical_document_versions "
        "WHERE medical_document_id=? ORDER BY revision_no DESC LIMIT 1")
        .bind(documentId).execute();
    auto row = result.fetchOne();
    if (!row) return nlohmann::json::object();
    return nlohmann::json::parse(row[0].get<std::string>());
}
}

nlohmann::json MedicalDocumentHandler::loadDocument(int orderId)
{
    auto result = dbManager->getSession()->sql(
        "SELECT md.id, md.document_no, md.order_id, md.owner_id, COALESCE(owner.name, ''), "
        "md.pet_id, COALESCE(p.pet_name, ''), COALESCE(p.pet_type, ''), COALESCE(p.pet_breed, ''), "
        "COALESCE(p.pet_age, ''), COALESCE(p.pet_sex, ''), md.doctor_id, COALESCE(doctor.name, ''), "
        "md.status, md.chief_complaint, md.present_illness, md.past_history, md.allergies, "
        "md.physical_exam, md.diagnosis, md.treatment_plan, md.discharge_advice, "
        "COALESCE(CAST(md.follow_up_at AS CHAR), ''), CAST(md.structured_data AS CHAR), "
        "md.revision_no, md.lock_version, md.template_version_id, "
        "COALESCE(CAST(md.finalized_at AS CHAR), ''), CAST(md.created_at AS CHAR), CAST(md.updated_at AS CHAR), "
        "COALESCE(o.order_totalprice, 0.0) "
        "FROM medical_documents md "
        "JOIN orders o ON o.id = md.order_id "
        "JOIN users owner ON owner.id = md.owner_id "
        "JOIN pets p ON p.id = md.pet_id "
        "JOIN users doctor ON doctor.id = md.doctor_id "
        "WHERE md.order_id = ? LIMIT 1")
        .bind(orderId)
        .execute();
    auto row = result.fetchOne();
    if (!row) return nlohmann::json();

    nlohmann::json document = {
        {"id", row[0].get<unsigned long long>()}, {"documentNo", row[1].get<std::string>()},
        {"orderId", row[2].get<int>()}, {"ownerId", row[3].get<int>()}, {"ownerName", row[4].get<std::string>()},
        {"petId", row[5].get<int>()}, {"petName", row[6].get<std::string>()}, {"petType", row[7].get<std::string>()},
        {"petBreed", row[8].get<std::string>()}, {"petAge", row[9].get<std::string>()}, {"petSex", row[10].get<std::string>()},
        {"doctorId", row[11].get<int>()}, {"doctorName", row[12].get<std::string>()}, {"status", row[13].get<std::string>()},
        {"chiefComplaint", row[14].get<std::string>()}, {"presentIllness", row[15].get<std::string>()},
        {"pastHistory", row[16].get<std::string>()}, {"allergies", row[17].get<std::string>()},
        {"physicalExam", row[18].get<std::string>()}, {"diagnosis", row[19].get<std::string>()},
        {"treatmentPlan", row[20].get<std::string>()}, {"dischargeAdvice", row[21].get<std::string>()},
        {"followUpAt", row[22].get<std::string>()}, {"structuredData", parseJsonColumn(row[23])},
        {"revisionNo", row[24].get<int>()}, {"lockVersion", row[25].get<int>()},
        {"templateVersionId", row[26].isNull() ? 0 : row[26].get<int>()}, {"finalizedAt", row[27].get<std::string>()},
        {"createdAt", row[28].get<std::string>()}, {"updatedAt", row[29].get<std::string>()},
        {"totalPrice", row[30].get<double>()},
    };

    auto prescriptions = dbManager->getSession()->sql(
        "SELECT id, COALESCE(medicine_id, 0), medicine_name, specification, unit, dosage, frequency, route, "
        "duration_days, quantity, instructions, unit_price, total_price, sort_order "
        "FROM medical_prescription_items WHERE medical_document_id = ? ORDER BY sort_order, id")
        .bind(document["id"].get<unsigned long long>())
        .execute();
    document["prescriptionItems"] = nlohmann::json::array();
    for (auto item : prescriptions)
    {
        document["prescriptionItems"].push_back({
            {"id", item[0].get<unsigned long long>()}, {"medicineId", item[1].get<int>()},
            {"medicineName", item[2].get<std::string>()}, {"specification", item[3].get<std::string>()},
            {"unit", item[4].get<std::string>()}, {"dosage", item[5].get<std::string>()},
            {"frequency", item[6].get<std::string>()}, {"route", item[7].get<std::string>()},
            {"durationDays", item[8].get<int>()}, {"quantity", item[9].get<int>()},
            {"instructions", item[10].get<std::string>()}, {"unitPrice", item[11].get<double>()},
            {"totalPrice", item[12].get<double>()}, {"sortOrder", item[13].get<int>()},
        });
    }
    return document;
}

nlohmann::json MedicalDocumentHandler::buildReportPayload(const nlohmann::json &document) const
{
    return {
        {"schemaVersion", "medical-document.v1"},
        {"hospital", {
            {"name", getEnvVar("HOSPITAL_NAME", "PetManager 宠物医院")},
            {"address", getEnvVar("HOSPITAL_ADDRESS", "")},
            {"phone", getEnvVar("HOSPITAL_PHONE", "")},
        }},
        {"document", {
            {"number", document.value("documentNo", "")}, {"orderNumber", document.value("orderId", 0)},
            {"issuedAt", document.value("finalizedAt", document.value("createdAt", ""))},
            {"status", document.value("status", "draft") == "draft" ? "草稿" : "已定稿"},
        }},
        {"owner", {{"id", document.value("ownerId", 0)}, {"name", document.value("ownerName", "")}}},
        {"pet", {
            {"id", document.value("petId", 0)}, {"name", document.value("petName", "")},
            {"species", document.value("petType", "")}, {"breed", document.value("petBreed", "")},
            {"age", document.value("petAge", "")}, {"sex", document.value("petSex", "")},
        }},
        {"doctor", {{"id", document.value("doctorId", 0)}, {"name", document.value("doctorName", "")}}},
        {"visit", {
            {"chiefComplaint", document.value("chiefComplaint", "")}, {"presentIllness", document.value("presentIllness", "")},
            {"pastHistory", document.value("pastHistory", "")}, {"allergies", document.value("allergies", "")},
            {"physicalExam", document.value("physicalExam", "")}, {"diagnosis", document.value("diagnosis", "")},
            {"treatmentPlan", document.value("treatmentPlan", "")}, {"dischargeAdvice", document.value("dischargeAdvice", "")},
            {"followUpAt", document.value("followUpAt", "")}, {"structuredData", document.value("structuredData", nlohmann::json::object())},
        }},
        {"prescription", {{"items", document.value("prescriptionItems", nlohmann::json::array())}, {"total", document.value("totalPrice", 0.0)}}},
    };
}

crow::response MedicalDocumentHandler::getByOrder(const crow::request &req, int orderId)
{
    try
    {
        if (!checkDbConnection()) return ResponseHelper::database_error(req);
        auto document = loadDocument(orderId);
        return document.empty() ? ResponseHelper::notFound(req, "Medical document not found") : ResponseHelper::success(req, document);
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::updateDraft(const crow::request &req, int orderId, int actorId)
{
    try
    {
        crow::response response;
        auto bodyOpt = validateRequest(req, response);
        if (!bodyOpt) return response;
        const auto &body = *bodyOpt;
        if (!textFieldsAreSafe(body) || !structuredDataIsSafe(body))
            return ResponseHelper::validation(req, "诊疗内容格式不正确或超过大小限制");
        const int lockVersion = body.value("lockVersion", -1);
        if (lockVersion < 0) return ResponseHelper::validation(req, "缺少 lockVersion");

        auto session = dbManager->getSession();
        TransactionGuard transaction(*session);
        auto update = session->sql(
            "UPDATE medical_documents SET chief_complaint=?, present_illness=?, past_history=?, allergies=?, "
            "physical_exam=?, diagnosis=?, treatment_plan=?, discharge_advice=?, follow_up_at=NULLIF(?, ''), "
            "structured_data=?, lock_version=lock_version+1 "
            "WHERE order_id=? AND status='draft' AND lock_version=?")
            .bind(bodyString(body, "chiefComplaint"), bodyString(body, "presentIllness"),
                  bodyString(body, "pastHistory"), bodyString(body, "allergies"), bodyString(body, "physicalExam"),
                  bodyString(body, "diagnosis"), bodyString(body, "treatmentPlan"), bodyString(body, "dischargeAdvice"),
                  bodyString(body, "followUpAt"), body.value("structuredData", nlohmann::json::object()).dump(),
                  orderId, lockVersion)
            .execute();
        if (update.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::fail(req, 409, ResponseCode::BusinessConflict, "诊疗单已被修改或已经定稿", ResponseErrorType::BusinessConflict, "Reload the latest document");
        }

        auto idResult = session->sql("SELECT id FROM medical_documents WHERE order_id=? LIMIT 1").bind(orderId).execute();
        auto idRow = idResult.fetchOne();
        const auto documentId = idRow[0].get<unsigned long long>();
        if (body.contains("prescriptionItems"))
        {
            if (!prescriptionInstructionsAreSafe(body["prescriptionItems"]))
                return ResponseHelper::validation(req, "处方用法格式不正确或超过大小限制");
            updatePrescriptionInstructions(*session, documentId, body["prescriptionItems"]);
        }
        transaction.commit();
        return ResponseHelper::success(req, loadDocument(orderId));
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::preview(const crow::request &req, int orderId)
{
    try
    {
        auto document = loadDocument(orderId);
        if (document.empty()) return ResponseHelper::notFound(req, "Medical document not found");
        auto templateResult = dbManager->getSession()->sql(
            "SELECT v.id, v.template_content FROM report_templates t "
            "JOIN report_template_versions v ON v.id=t.current_version_id "
            "WHERE t.document_type='medical_document' AND t.status='published' LIMIT 1").execute();
        auto templateRow = templateResult.fetchOne();
        if (!templateRow) return ResponseHelper::unavailable(req, "没有已发布的诊疗单模板");
        auto payload = buildReportPayload(document);
        payload["document"]["status"] = "草稿预览";
        return ResponseHelper::success(req, {
            {"templateVersionId", templateRow[0].get<int>()},
            {"html", Reporting::hardenReportHtml(Reporting::renderTemplate(templateRow[1].get<std::string>(), payload.dump()))},
        });
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::finalize(const crow::request &req, int orderId, int actorId)
{
    try
    {
        auto document = loadDocument(orderId);
        if (document.empty()) return ResponseHelper::notFound(req, "Medical document not found");
        if (document.value("diagnosis", "").empty()) return ResponseHelper::validation(req, "定稿前必须填写诊断");
        if (document.value("status", "") == "voided") return ResponseHelper::validation(req, "已作废诊疗单不能定稿");

        auto session = dbManager->getSession();
        unsigned long long versionId = 0;
        int templateVersionId = document.value("templateVersionId", 0);
        nlohmann::json payload;

        if (document.value("status", "") == "draft")
        {
            auto templateResult = session->sql(
                "SELECT v.id FROM report_templates t JOIN report_template_versions v ON v.id=t.current_version_id "
                "WHERE t.document_type='medical_document' AND t.status='published' AND v.status='published' LIMIT 1").execute();
            auto templateRow = templateResult.fetchOne();
            if (!templateRow) return ResponseHelper::unavailable(req, "没有已发布的诊疗单模板");
            templateVersionId = templateRow[0].get<int>();
            document["status"] = "finalized";
            document["revisionNo"] = 1;
            document["finalizedAt"] = getCreateTime();
            payload = buildReportPayload(document);
            payload["document"]["status"] = "已定稿";
            const std::string snapshot = payload.dump();

            TransactionGuard transaction(*session);
            auto update = session->sql(
                "UPDATE medical_documents SET status='finalized', revision_no=1, template_version_id=?, "
                "finalized_by=?, finalized_at=UTC_TIMESTAMP(), lock_version=lock_version+1 "
                "WHERE id=? AND status='draft'")
                .bind(templateVersionId, actorId, document["id"].get<unsigned long long>()).execute();
            if (update.getAffectedItemsCount() == 0)
                return ResponseHelper::fail(req, 409, ResponseCode::BusinessConflict, "诊疗单状态已变化", ResponseErrorType::BusinessConflict);
            auto versionResult = session->sql(
                "INSERT INTO medical_document_versions (medical_document_id, revision_no, snapshot_json, change_reason, content_hash, created_by) "
                "VALUES (?, 1, ?, '首次定稿', ?, ?)")
                .bind(document["id"].get<unsigned long long>(), snapshot, sha256_hash(snapshot), actorId).execute();
            versionId = versionResult.getAutoIncrementValue();
            transaction.commit();
        }
        else
        {
            auto versionResult = session->sql(
                "SELECT id, CAST(snapshot_json AS CHAR) FROM medical_document_versions "
                "WHERE medical_document_id=? ORDER BY revision_no DESC LIMIT 1")
                .bind(document["id"].get<unsigned long long>()).execute();
            auto versionRow = versionResult.fetchOne();
            if (!versionRow) return ResponseHelper::system_error(req, "定稿版本不存在");
            versionId = versionRow[0].get<unsigned long long>();
            payload = nlohmann::json::parse(versionRow[1].get<std::string>());
        }

        auto artifactResult = session->sql(
            "SELECT id, format, sha256, byte_size, CAST(generated_at AS CHAR) FROM report_artifacts "
            "WHERE medical_document_version_id=? AND template_version_id=? AND format='pdf' LIMIT 1")
            .bind(versionId, templateVersionId).execute();
        auto artifactRow = artifactResult.fetchOne();
        if (artifactRow) return ResponseHelper::success(req, {{"document", loadDocument(orderId)}, {"artifact", artifactJson(artifactRow)}});

        auto templateResult = session->sql("SELECT template_content FROM report_template_versions WHERE id=? LIMIT 1")
            .bind(templateVersionId).execute();
        auto templateRow = templateResult.fetchOne();
        if (!templateRow) return ResponseHelper::system_error(req, "诊疗单模板版本不存在");
        const std::string html = Reporting::renderTemplate(templateRow[0].get<std::string>(), payload.dump());
        const std::string storageKey = "medical/" + std::to_string(document["id"].get<unsigned long long>()) +
                                       "/revision-" + std::to_string(document.value("revisionNo", 1)) + ".pdf";
        Reporting::ChromeReportRenderer renderer;
        const auto rendered = renderer.renderPdf(html, storageKey);
        if (!rendered.success)
            return ResponseHelper::unavailable(req, "诊疗单已定稿，但 PDF 生成失败：" + rendered.error);

        unsigned long long artifactId = 0;
        try
        {
            auto insert = session->sql(
                "INSERT INTO report_artifacts (medical_document_version_id, template_version_id, format, storage_key, sha256, payload_hash, byte_size, generated_by) "
                "VALUES (?, ?, 'pdf', ?, ?, ?, ?, ?)")
                .bind(versionId, templateVersionId, rendered.storageKey, rendered.sha256, sha256_hash(payload.dump()),
                      static_cast<unsigned long long>(rendered.byteSize), actorId).execute();
            artifactId = insert.getAutoIncrementValue();
        }
        catch (const mysqlx::Error &)
        {
            auto concurrentResult = session->sql(
                "SELECT id, format, sha256, byte_size, CAST(generated_at AS CHAR) FROM report_artifacts "
                "WHERE medical_document_version_id=? AND template_version_id=? AND format='pdf' LIMIT 1")
                .bind(versionId, templateVersionId).execute();
            auto concurrentRow = concurrentResult.fetchOne();
            if (!concurrentRow)
            {
                std::error_code removeError;
                std::filesystem::remove(rendered.absolutePath, removeError);
                throw;
            }
            return ResponseHelper::success(req, {{"document", loadDocument(orderId)}, {"artifact", artifactJson(concurrentRow)}});
        }
        return ResponseHelper::success(req, {
            {"document", loadDocument(orderId)},
            {"artifact", {{"id", artifactId}, {"format", "pdf"}, {"sha256", rendered.sha256}, {"byteSize", rendered.byteSize}}},
        });
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::amend(const crow::request &req, int orderId, int actorId)
{
    try
    {
        crow::response response;
        auto bodyOpt = validateRequest(req, response);
        if (!bodyOpt) return response;
        const auto &body = *bodyOpt;
        const std::string reason = bodyString(body, "reason");
        if (reason.empty() || reason.size() > 500) return ResponseHelper::validation(req, "修订原因不能为空且不能超过 500 字节");
        if (!textFieldsAreSafe(body) || !structuredDataIsSafe(body))
            return ResponseHelper::validation(req, "诊疗内容格式不正确或超过大小限制");
        if (bodyString(body, "diagnosis").empty()) return ResponseHelper::validation(req, "修订后的诊断不能为空");

        auto current = loadDocument(orderId);
        if (current.empty()) return ResponseHelper::notFound(req, "Medical document not found");
        if (current.value("status", "") != "finalized" && current.value("status", "") != "amended")
            return ResponseHelper::fail(req, 409, ResponseCode::BusinessConflict, "只有已定稿诊疗单可以修订", ResponseErrorType::BusinessConflict);
        const int lockVersion = body.value("lockVersion", -1);
        if (lockVersion < 0) return ResponseHelper::validation(req, "缺少 lockVersion");
        if (body.contains("prescriptionItems") && !prescriptionInstructionsAreSafe(body["prescriptionItems"]))
            return ResponseHelper::validation(req, "处方用法格式不正确或超过大小限制");

        auto session = dbManager->getSession();
        auto templateResult = session->sql(
            "SELECT v.id FROM report_templates t JOIN report_template_versions v ON v.id=t.current_version_id "
            "WHERE t.document_type='medical_document' AND t.status='published' AND v.status='published' LIMIT 1").execute();
        auto templateRow = templateResult.fetchOne();
        if (!templateRow) return ResponseHelper::unavailable(req, "没有已发布的诊疗单模板");
        const int templateVersionId = templateRow[0].get<int>();
        const int revisionNo = current.value("revisionNo", 0) + 1;
        const auto documentId = current["id"].get<unsigned long long>();

        TransactionGuard transaction(*session);
        const auto previousSnapshot = loadLatestSnapshot(*session, documentId);
        if (previousSnapshot.empty()) return ResponseHelper::system_error(req, "上一版诊疗单快照不存在");
        auto update = session->sql(
            "UPDATE medical_documents SET status='amended', chief_complaint=?, present_illness=?, past_history=?, allergies=?, "
            "physical_exam=?, diagnosis=?, treatment_plan=?, discharge_advice=?, follow_up_at=NULLIF(?, ''), structured_data=?, "
            "revision_no=?, template_version_id=?, lock_version=lock_version+1 "
            "WHERE id=? AND status IN ('finalized','amended') AND lock_version=?")
            .bind(bodyString(body, "chiefComplaint"), bodyString(body, "presentIllness"), bodyString(body, "pastHistory"),
                  bodyString(body, "allergies"), bodyString(body, "physicalExam"), bodyString(body, "diagnosis"),
                  bodyString(body, "treatmentPlan"), bodyString(body, "dischargeAdvice"), bodyString(body, "followUpAt"),
                  body.value("structuredData", nlohmann::json::object()).dump(), revisionNo, templateVersionId,
                  documentId, lockVersion).execute();
        if (update.getAffectedItemsCount() == 0)
            return ResponseHelper::fail(req, 409, ResponseCode::BusinessConflict, "诊疗单已被其他人修改", ResponseErrorType::BusinessConflict, "Reload the latest document");

        if (body.contains("prescriptionItems"))
        {
            updatePrescriptionInstructions(*session, documentId, body["prescriptionItems"]);
        }
        auto amended = loadDocument(orderId);
        amended["status"] = "amended";
        amended["revisionNo"] = revisionNo;
        const auto changedPayload = buildReportPayload(amended);
        auto payload = previousSnapshot;
        payload["document"]["status"] = "已修订";
        payload["visit"] = changedPayload["visit"];
        payload["prescription"]["items"] = changedPayload["prescription"]["items"];
        const std::string snapshot = payload.dump();
        session->sql(
            "INSERT INTO medical_document_versions (medical_document_id, revision_no, snapshot_json, change_reason, content_hash, created_by) "
            "VALUES (?, ?, ?, ?, ?, ?)")
            .bind(documentId, revisionNo, snapshot, reason, sha256_hash(snapshot), actorId).execute();
        transaction.commit();
        return finalize(req, orderId, actorId);
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::voidDocument(const crow::request &req, int orderId, int actorId)
{
    try
    {
        crow::response response;
        auto bodyOpt = validateRequest(req, response);
        if (!bodyOpt) return response;
        const std::string reason = bodyString(*bodyOpt, "reason");
        if (reason.empty() || reason.size() > 500) return ResponseHelper::validation(req, "作废原因不能为空且不能超过 500 字节");
        const int lockVersion = bodyOpt->value("lockVersion", -1);
        if (lockVersion < 0) return ResponseHelper::validation(req, "缺少 lockVersion");

        auto current = loadDocument(orderId);
        if (current.empty()) return ResponseHelper::notFound(req, "Medical document not found");
        if (current.value("status", "") != "finalized" && current.value("status", "") != "amended")
            return ResponseHelper::fail(req, 409, ResponseCode::BusinessConflict, "只有已定稿诊疗单可以作废", ResponseErrorType::BusinessConflict);
        const int revisionNo = current.value("revisionNo", 0) + 1;
        const auto documentId = current["id"].get<unsigned long long>();

        auto session = dbManager->getSession();
        TransactionGuard transaction(*session);
        auto payload = loadLatestSnapshot(*session, documentId);
        if (payload.empty()) return ResponseHelper::system_error(req, "上一版诊疗单快照不存在");
        payload["document"]["status"] = "已作废";
        const std::string snapshot = payload.dump();
        auto update = session->sql(
            "UPDATE medical_documents SET status='voided', revision_no=?, voided_by=?, voided_at=UTC_TIMESTAMP(), "
            "void_reason=?, lock_version=lock_version+1 WHERE id=? AND status IN ('finalized','amended') AND lock_version=?")
            .bind(revisionNo, actorId, reason, documentId, lockVersion).execute();
        if (update.getAffectedItemsCount() == 0)
            return ResponseHelper::fail(req, 409, ResponseCode::BusinessConflict, "诊疗单已被其他人修改", ResponseErrorType::BusinessConflict, "Reload the latest document");
        session->sql(
            "INSERT INTO medical_document_versions (medical_document_id, revision_no, snapshot_json, change_reason, content_hash, created_by) "
            "VALUES (?, ?, ?, ?, ?, ?)")
            .bind(documentId, revisionNo, snapshot, "作废：" + reason, sha256_hash(snapshot), actorId).execute();
        transaction.commit();
        return ResponseHelper::success(req, loadDocument(orderId));
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::downloadLatestPdf(const crow::request &req, int orderId)
{
    try
    {
        auto result = dbManager->getSession()->sql(
            "SELECT a.storage_key, md.document_no FROM report_artifacts a "
            "JOIN medical_document_versions v ON v.id=a.medical_document_version_id "
            "JOIN medical_documents md ON md.id=v.medical_document_id "
            "WHERE md.order_id=? AND md.status IN ('finalized','amended') AND v.revision_no=md.revision_no "
            "AND a.format='pdf' ORDER BY a.generated_at DESC LIMIT 1")
            .bind(orderId).execute();
        auto row = result.fetchOne();
        if (!row) return ResponseHelper::notFound(req, "PDF artifact not found");
        const std::string key = row[0].get<std::string>();
        if (key.empty() || key.front() == '/' || key.find("..") != std::string::npos)
            return ResponseHelper::system_error(req, "Invalid report storage key");
        const std::filesystem::path path = std::filesystem::path(Reporting::reportStorageRoot()) / key;
        std::ifstream file(path, std::ios::binary);
        if (!file) return ResponseHelper::notFound(req, "PDF artifact file not found");
        std::ostringstream buffer;
        buffer << file.rdbuf();
        crow::response response(buffer.str());
        response.code = 200;
        response.set_header("Content-Type", "application/pdf");
        response.set_header("Content-Disposition", "inline; filename=\"" + row[1].get<std::string>() + ".pdf\"");
        response.set_header("Cache-Control", "private, no-store");
        return response;
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::listVersions(const crow::request &req, int orderId)
{
    try
    {
        auto result = dbManager->getSession()->sql(
            "SELECT v.id, v.revision_no, v.change_reason, v.content_hash, CAST(v.created_at AS CHAR), "
            "CAST(v.snapshot_json AS CHAR), COALESCE(a.id, 0), COALESCE(a.sha256, ''), COALESCE(a.byte_size, 0) "
            "FROM medical_document_versions v "
            "JOIN medical_documents md ON md.id=v.medical_document_id "
            "LEFT JOIN report_artifacts a ON a.medical_document_version_id=v.id AND a.format='pdf' "
            "WHERE md.order_id=? ORDER BY v.revision_no DESC, a.generated_at DESC")
            .bind(orderId).execute();
        nlohmann::json versions = nlohmann::json::array();
        int lastRevision = -1;
        for (auto row : result)
        {
            const int revisionNo = row[1].get<int>();
            if (revisionNo == lastRevision) continue;
            lastRevision = revisionNo;
            const auto snapshot = nlohmann::json::parse(row[5].get<std::string>());
            versions.push_back({
                {"id", row[0].get<unsigned long long>()}, {"revisionNo", revisionNo},
                {"changeReason", row[2].get<std::string>()}, {"contentHash", row[3].get<std::string>()},
                {"createdAt", row[4].get<std::string>()}, {"snapshot", snapshot},
                {"hasPdf", row[6].get<unsigned long long>() > 0}, {"pdfSha256", row[7].get<std::string>()},
                {"pdfByteSize", row[8].get<unsigned long long>()},
            });
        }
        return ResponseHelper::success(req, versions);
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::downloadVersionPdf(const crow::request &req, int orderId, int revisionNo)
{
    try
    {
        auto result = dbManager->getSession()->sql(
            "SELECT a.storage_key, md.document_no FROM report_artifacts a "
            "JOIN medical_document_versions v ON v.id=a.medical_document_version_id "
            "JOIN medical_documents md ON md.id=v.medical_document_id "
            "WHERE md.order_id=? AND v.revision_no=? AND a.format='pdf' ORDER BY a.generated_at DESC LIMIT 1")
            .bind(orderId, revisionNo).execute();
        auto row = result.fetchOne();
        if (!row) return ResponseHelper::notFound(req, "PDF artifact not found");
        const std::string key = row[0].get<std::string>();
        if (key.empty() || key.front() == '/' || key.find("..") != std::string::npos)
            return ResponseHelper::system_error(req, "Invalid report storage key");
        const std::filesystem::path path = std::filesystem::path(Reporting::reportStorageRoot()) / key;
        std::ifstream file(path, std::ios::binary);
        if (!file) return ResponseHelper::notFound(req, "PDF artifact file not found");
        std::ostringstream buffer;
        buffer << file.rdbuf();
        crow::response response(buffer.str());
        response.code = 200;
        response.set_header("Content-Type", "application/pdf");
        response.set_header("Content-Disposition", "inline; filename=\"" + row[1].get<std::string>() + "-r" + std::to_string(revisionNo) + ".pdf\"");
        response.set_header("Cache-Control", "private, no-store");
        return response;
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::listForOwner(const crow::request &req, int ownerId)
{
    try
    {
        auto result = dbManager->getSession()->sql(
            "SELECT md.id, md.document_no, md.status, md.revision_no, "
            "COALESCE(CAST(md.finalized_at AS CHAR), ''), CAST(v.snapshot_json AS CHAR), md.void_reason "
            "FROM medical_documents md "
            "JOIN medical_document_versions v ON v.medical_document_id=md.id AND v.revision_no=md.revision_no "
            "WHERE md.owner_id=? AND md.status IN ('finalized','amended','voided') "
            "ORDER BY md.finalized_at DESC, md.id DESC")
            .bind(ownerId).execute();
        nlohmann::json data = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json snapshot = nlohmann::json::parse(row[5].get<std::string>());
            data.push_back({
                {"id", row[0].get<unsigned long long>()}, {"documentNo", row[1].get<std::string>()},
                {"status", row[2].get<std::string>()}, {"revisionNo", row[3].get<int>()},
                {"finalizedAt", row[4].get<std::string>()}, {"voidReason", row[6].get<std::string>()},
                {"pet", snapshot.value("pet", nlohmann::json::object())},
                {"doctor", snapshot.value("doctor", nlohmann::json::object())},
                {"diagnosis", snapshot.value("visit", nlohmann::json::object()).value("diagnosis", "")},
            });
        }
        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::getForOwner(const crow::request &req, int documentId, int ownerId)
{
    try
    {
        auto result = dbManager->getSession()->sql(
            "SELECT md.document_no, md.status, md.revision_no, md.void_reason, "
            "COALESCE(CAST(md.finalized_at AS CHAR), ''), CAST(v.snapshot_json AS CHAR) "
            "FROM medical_documents md "
            "JOIN medical_document_versions v ON v.medical_document_id=md.id AND v.revision_no=md.revision_no "
            "WHERE md.id=? AND md.owner_id=? AND md.status IN ('finalized','amended','voided') LIMIT 1")
            .bind(documentId, ownerId).execute();
        auto row = result.fetchOne();
        if (!row) return ResponseHelper::notFound(req, "Medical document not found");
        return ResponseHelper::success(req, {
            {"id", documentId}, {"documentNo", row[0].get<std::string>()}, {"status", row[1].get<std::string>()},
            {"revisionNo", row[2].get<int>()}, {"voidReason", row[3].get<std::string>()},
            {"finalizedAt", row[4].get<std::string>()}, {"snapshot", nlohmann::json::parse(row[5].get<std::string>())},
        });
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response MedicalDocumentHandler::downloadForOwner(const crow::request &req, int documentId, int ownerId)
{
    try
    {
        auto result = dbManager->getSession()->sql(
            "SELECT a.storage_key, md.document_no FROM report_artifacts a "
            "JOIN medical_document_versions v ON v.id=a.medical_document_version_id "
            "JOIN medical_documents md ON md.id=v.medical_document_id "
            "WHERE md.id=? AND md.owner_id=? AND md.status IN ('finalized','amended') "
            "AND v.revision_no=md.revision_no AND a.format='pdf' "
            "ORDER BY a.generated_at DESC LIMIT 1")
            .bind(documentId, ownerId).execute();
        auto row = result.fetchOne();
        if (!row) return ResponseHelper::notFound(req, "PDF artifact not found");
        const std::string key = row[0].get<std::string>();
        if (key.empty() || key.front() == '/' || key.find("..") != std::string::npos)
            return ResponseHelper::system_error(req, "Invalid report storage key");
        const std::filesystem::path path = std::filesystem::path(Reporting::reportStorageRoot()) / key;
        std::ifstream file(path, std::ios::binary);
        if (!file) return ResponseHelper::notFound(req, "PDF artifact file not found");
        std::ostringstream buffer;
        buffer << file.rdbuf();
        crow::response response(buffer.str());
        response.code = 200;
        response.set_header("Content-Type", "application/pdf");
        response.set_header("Content-Disposition", "inline; filename=\"" + row[1].get<std::string>() + ".pdf\"");
        response.set_header("Cache-Control", "private, no-store");
        return response;
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}
