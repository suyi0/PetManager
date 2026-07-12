#include "ReportTemplateHandler.h"
#include "../../../services/reporting/ReportRenderer.h"

#include <algorithm>
#include <cctype>

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

bool containsForbiddenTemplateCapability(const std::string &content)
{
    std::string normalized = content;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch)
                   { return static_cast<char>(std::tolower(ch)); });
    const std::string forbiddenTags[] = {
        "script", "meta", "base", "iframe", "object", "embed", "link", "form", "input", "button", "a",
    };
    for (const auto &tag : forbiddenTags)
    {
        std::size_t position = normalized.find("<" + tag);
        while (position != std::string::npos)
        {
            const std::size_t boundary = position + tag.size() + 1;
            if (boundary < normalized.size() &&
                (normalized[boundary] == '>' || normalized[boundary] == '/' ||
                 std::isspace(static_cast<unsigned char>(normalized[boundary])))) return true;
            position = normalized.find("<" + tag, boundary);
        }
    }
    const std::string forbidden[] = {
        "javascript:", "http://", "https://", "file://", "fetch(", "xmlhttprequest", "@import", "url(", "src=//", "href=//",
    };
    for (const auto &token : forbidden)
    {
        if (normalized.find(token) != std::string::npos) return true;
    }
    return false;
}

nlohmann::json sampleMedicalDocumentPayload()
{
    return {
        {"schemaVersion", "medical-document.v1"},
        {"hospital", {{"name", "PetManager 宠物医院"}, {"address", "示例地址"}, {"phone", "000-00000000"}}},
        {"document", {{"number", "MD-20260711-00000001"}, {"orderNumber", 1}, {"issuedAt", "2026-07-11 09:30:00"}, {"status", "模板预览"}}},
        {"owner", {{"id", 0}, {"name", "示例主人"}}},
        {"pet", {{"id", 0}, {"name", "示例宠物"}, {"species", "犬"}, {"breed", "示例品种"}, {"age", "3岁"}, {"sex", "雄"}}},
        {"doctor", {{"id", 0}, {"name", "示例医生"}}},
        {"visit", {
            {"chiefComplaint", "呕吐、食欲下降两天。"},
            {"presentIllness", "症状出现后未自行用药。"},
            {"pastHistory", "既往无重大疾病。"},
            {"allergies", "未发现。"},
            {"physicalExam", "精神一般，腹部轻度紧张。"},
            {"diagnosis", "示例诊断。"},
            {"treatmentPlan", "对症治疗并观察。"},
            {"dischargeAdvice", "少量多次饮水，如加重及时复诊。"},
            {"followUpAt", "2026-07-14 09:30:00"},
            {"structuredData", {{"vitals", {{"weight", "12.6"}, {"temperature", "39.1"}}}}},
        }},
        {"prescription", {
            {"items", nlohmann::json::array({
                {{"medicineName", "示例药品 A"}, {"specification", "0.5g x 12片"}, {"unit", "盒"}, {"dosage", "0.5片"}, {"frequency", "每日2次"}, {"route", "口服"}, {"durationDays", 3}, {"quantity", 1}, {"instructions", "餐后服用"}},
                {{"medicineName", "示例药品 B"}, {"specification", "10mg"}, {"unit", "盒"}, {"dosage", "1片"}, {"frequency", "每日1次"}, {"route", "口服"}, {"durationDays", 2}, {"quantity", 1}, {"instructions", "按医嘱使用"}},
            })},
            {"total", 86.0},
        }},
    };
}
}

crow::response ReportTemplateHandler::dataContract(const crow::request &req)
{
    const nlohmann::json fields = nlohmann::json::array({
        {{"path", "hospital.name"}, {"label", "医院名称"}, {"type", "string"}, {"example", "PetManager 宠物医院"}, {"sensitivity", "internal"}},
        {{"path", "hospital.address"}, {"label", "医院地址"}, {"type", "string"}},
        {{"path", "hospital.phone"}, {"label", "医院电话"}, {"type", "string"}},
        {{"path", "document.number"}, {"label", "诊疗单号"}, {"type", "string"}},
        {{"path", "document.orderNumber"}, {"label", "订单编号"}, {"type", "number"}},
        {{"path", "document.issuedAt"}, {"label", "定稿时间"}, {"type", "datetime"}},
        {{"path", "owner.name"}, {"label", "主人姓名"}, {"type", "string"}, {"example", "示例主人"}, {"sensitivity", "personal"}},
        {{"path", "pet.name"}, {"label", "宠物姓名"}, {"type", "string"}},
        {{"path", "pet.species"}, {"label", "宠物种类"}, {"type", "string"}},
        {{"path", "pet.breed"}, {"label", "宠物品种"}, {"type", "string"}},
        {{"path", "pet.age"}, {"label", "宠物年龄"}, {"type", "string"}},
        {{"path", "pet.sex"}, {"label", "宠物性别"}, {"type", "string"}},
        {{"path", "doctor.name"}, {"label", "接诊医生"}, {"type", "string"}},
        {{"path", "visit.chiefComplaint"}, {"label", "主诉"}, {"type", "multiline"}},
        {{"path", "visit.presentIllness"}, {"label", "现病史"}, {"type", "multiline"}},
        {{"path", "visit.pastHistory"}, {"label", "既往史"}, {"type", "multiline"}},
        {{"path", "visit.allergies"}, {"label", "过敏史"}, {"type", "multiline"}},
        {{"path", "visit.physicalExam"}, {"label", "检查记录"}, {"type", "multiline"}},
        {{"path", "visit.diagnosis"}, {"label", "诊断"}, {"type", "multiline"}, {"example", "示例诊断"}, {"sensitivity", "medical"}},
        {{"path", "visit.treatmentPlan"}, {"label", "治疗方案"}, {"type", "multiline"}},
        {{"path", "visit.dischargeAdvice"}, {"label", "离院医嘱"}, {"type", "multiline"}},
        {{"path", "visit.followUpAt"}, {"label", "复诊时间"}, {"type", "datetime"}},
        {{"path", "prescription.items"}, {"label", "处方明细"}, {"type", "collection"}, {"itemFields", {
            "medicineName", "specification", "unit", "dosage", "frequency", "route", "durationDays", "quantity", "instructions"
        }}},
        {{"path", "prescription.total"}, {"label", "费用合计"}, {"type", "currency"}},
    });
    return ResponseHelper::success(req, {{"version", "medical-document.v1"}, {"fields", fields}});
}

crow::response ReportTemplateHandler::list(const crow::request &req)
{
    try
    {
        auto result = dbManager->getSession()->sql(
            "SELECT t.id, t.code, t.name, t.document_type, t.paper_size, t.orientation, t.status, "
            "COALESCE(t.current_version_id, 0), COALESCE(v.version_no, 0), CAST(t.updated_at AS CHAR) "
            "FROM report_templates t LEFT JOIN report_template_versions v ON v.id=t.current_version_id "
            "ORDER BY t.document_type, t.name").execute();
        nlohmann::json data = nlohmann::json::array();
        for (auto row : result)
        {
            data.push_back({
                {"id", row[0].get<int>()}, {"code", row[1].get<std::string>()}, {"name", row[2].get<std::string>()},
                {"documentType", row[3].get<std::string>()}, {"paperSize", row[4].get<std::string>()},
                {"orientation", row[5].get<std::string>()}, {"status", row[6].get<std::string>()},
                {"currentVersionId", row[7].get<int>()}, {"currentVersionNo", row[8].get<int>()},
                {"updatedAt", row[9].get<std::string>()},
            });
        }
        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response ReportTemplateHandler::versions(const crow::request &req, int templateId)
{
    try
    {
        auto result = dbManager->getSession()->sql(
            "SELECT id, version_no, engine, data_contract_version, status, COALESCE(CAST(published_at AS CHAR), ''), "
            "CAST(created_at AS CHAR) FROM report_template_versions WHERE template_id=? ORDER BY version_no DESC")
            .bind(templateId).execute();
        nlohmann::json data = nlohmann::json::array();
        for (auto row : result)
        {
            data.push_back({
                {"id", row[0].get<int>()}, {"versionNo", row[1].get<int>()}, {"engine", row[2].get<std::string>()},
                {"dataContractVersion", row[3].get<std::string>()}, {"status", row[4].get<std::string>()},
                {"publishedAt", row[5].get<std::string>()}, {"createdAt", row[6].get<std::string>()},
            });
        }
        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response ReportTemplateHandler::getVersion(const crow::request &req, int templateId, int versionId)
{
    try
    {
        auto result = dbManager->getSession()->sql(
            "SELECT id, version_no, engine, template_content, data_contract_version, status, "
            "COALESCE(CAST(published_at AS CHAR), ''), CAST(created_at AS CHAR) "
            "FROM report_template_versions WHERE id=? AND template_id=? LIMIT 1")
            .bind(versionId, templateId).execute();
        auto row = result.fetchOne();
        if (!row) return ResponseHelper::notFound(req, "Report template version not found");
        return ResponseHelper::success(req, {
            {"id", row[0].get<int>()}, {"versionNo", row[1].get<int>()}, {"engine", row[2].get<std::string>()},
            {"templateContent", row[3].get<std::string>()}, {"dataContractVersion", row[4].get<std::string>()},
            {"status", row[5].get<std::string>()}, {"publishedAt", row[6].get<std::string>()}, {"createdAt", row[7].get<std::string>()},
        });
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response ReportTemplateHandler::createVersion(const crow::request &req, int templateId, int actorId)
{
    try
    {
        crow::response response;
        auto bodyOpt = validateRequest(req, response);
        if (!bodyOpt) return response;
        const std::string content = bodyOpt->value("templateContent", "");
        if (content.empty() || content.size() > 1024 * 1024)
            return ResponseHelper::validation(req, "模板内容不能为空且不能超过 1MB");
        if (containsForbiddenTemplateCapability(content))
            return ResponseHelper::validation(req, "模板不能包含脚本、外部网络资源或文件协议");

        auto session = dbManager->getSession();
        TransactionGuard transaction(*session);
        auto templateResult = session->sql("SELECT id FROM report_templates WHERE id=? FOR UPDATE").bind(templateId).execute();
        if (!templateResult.fetchOne()) return ResponseHelper::notFound(req, "Report template not found");
        auto nextResult = session->sql("SELECT COALESCE(MAX(version_no), 0) + 1 FROM report_template_versions WHERE template_id=?")
            .bind(templateId).execute();
        const int nextVersion = nextResult.fetchOne()[0].get<int>();
        auto result = session->sql(
            "INSERT INTO report_template_versions (template_id, version_no, engine, template_content, data_contract_version, status, created_by) "
            "VALUES (?, ?, 'html', ?, 'medical-document.v1', 'draft', ?)")
            .bind(templateId, nextVersion, content, actorId).execute();
        transaction.commit();
        return ResponseHelper::created(req, {{"id", result.getAutoIncrementValue()}, {"status", "draft"}});
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response ReportTemplateHandler::preview(const crow::request &req, int templateId)
{
    try
    {
        crow::response response;
        auto bodyOpt = validateRequest(req, response);
        if (!bodyOpt) return response;
        std::string content = bodyOpt->value("templateContent", "");
        const int versionId = bodyOpt->value("versionId", 0);
        if (content.empty() && versionId > 0)
        {
            auto result = dbManager->getSession()->sql(
                "SELECT template_content FROM report_template_versions WHERE id=? AND template_id=? LIMIT 1")
                .bind(versionId, templateId).execute();
            auto row = result.fetchOne();
            if (!row) return ResponseHelper::notFound(req, "Report template version not found");
            content = row[0].get<std::string>();
        }
        if (content.empty() || content.size() > 1024 * 1024)
            return ResponseHelper::validation(req, "模板内容不能为空且不能超过 1MB");
        if (containsForbiddenTemplateCapability(content))
            return ResponseHelper::validation(req, "模板不能包含脚本、外部网络资源或文件协议");

        auto templateResult = dbManager->getSession()->sql("SELECT 1 FROM report_templates WHERE id=? LIMIT 1")
            .bind(templateId).execute();
        if (!templateResult.fetchOne()) return ResponseHelper::notFound(req, "Report template not found");
        return ResponseHelper::success(req, {
            {"html", Reporting::hardenReportHtml(Reporting::renderTemplate(content, sampleMedicalDocumentPayload().dump()))},
            {"sampleData", sampleMedicalDocumentPayload()},
        });
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}

crow::response ReportTemplateHandler::publish(const crow::request &req, int templateId, int actorId)
{
    try
    {
        crow::response response;
        auto bodyOpt = validateRequest(req, response);
        if (!bodyOpt) return response;
        const int versionId = bodyOpt->value("versionId", 0);
        if (versionId <= 0) return ResponseHelper::validation(req, "versionId 无效");

        auto session = dbManager->getSession();
        TransactionGuard transaction(*session);
        auto templateRow = session->sql("SELECT id FROM report_templates WHERE id=? FOR UPDATE")
            .bind(templateId).execute();
        if (!templateRow.fetchOne()) return ResponseHelper::notFound(req, "Report template not found");
        auto exists = session->sql("SELECT template_content FROM report_template_versions WHERE id=? AND template_id=? LIMIT 1")
            .bind(versionId, templateId).execute();
        auto versionRow = exists.fetchOne();
        if (!versionRow) return ResponseHelper::notFound(req, "Report template version not found");
        if (containsForbiddenTemplateCapability(versionRow[0].get<std::string>()))
            return ResponseHelper::validation(req, "模板包含脚本、导航、外部资源或文件协议，不能发布");
        session->sql("UPDATE report_template_versions SET status='archived' WHERE template_id=? AND status='published'")
            .bind(templateId).execute();
        session->sql("UPDATE report_template_versions SET status='published', published_by=?, published_at=UTC_TIMESTAMP() WHERE id=?")
            .bind(actorId, versionId).execute();
        session->sql("UPDATE report_templates SET current_version_id=?, status='published' WHERE id=?")
            .bind(versionId, templateId).execute();
        transaction.commit();
        return ResponseHelper::success(req, {{"templateId", templateId}, {"currentVersionId", versionId}});
    }
    catch (const std::exception &error)
    {
        return ResponseHelper::system_error(req, error.what());
    }
}
