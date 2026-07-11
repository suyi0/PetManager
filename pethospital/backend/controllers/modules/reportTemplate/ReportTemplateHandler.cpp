#include "ReportTemplateHandler.h"

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
    const std::string forbidden[] = {
        "<script", "javascript:", "http://", "https://", "file://", "fetch(", "XMLHttpRequest", "@import",
    };
    for (const auto &token : forbidden)
    {
        if (content.find(token) != std::string::npos) return true;
    }
    return false;
}
}

crow::response ReportTemplateHandler::dataContract(const crow::request &req)
{
    const nlohmann::json fields = nlohmann::json::array({
        {{"path", "hospital.name"}, {"label", "医院名称"}, {"type", "string"}},
        {{"path", "hospital.address"}, {"label", "医院地址"}, {"type", "string"}},
        {{"path", "hospital.phone"}, {"label", "医院电话"}, {"type", "string"}},
        {{"path", "document.number"}, {"label", "诊疗单号"}, {"type", "string"}},
        {{"path", "document.orderNumber"}, {"label", "订单编号"}, {"type", "number"}},
        {{"path", "document.issuedAt"}, {"label", "定稿时间"}, {"type", "datetime"}},
        {{"path", "owner.name"}, {"label", "主人姓名"}, {"type", "string"}},
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
        {{"path", "visit.diagnosis"}, {"label", "诊断"}, {"type", "multiline"}},
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

        auto result = dbManager->getSession()->sql(
            "INSERT INTO report_template_versions (template_id, version_no, engine, template_content, data_contract_version, status, created_by) "
            "SELECT id, COALESCE((SELECT MAX(v.version_no)+1 FROM report_template_versions v WHERE v.template_id=?), 1), "
            "'html', ?, 'medical-document.v1', 'draft', ? FROM report_templates WHERE id=?")
            .bind(templateId, content, actorId, templateId).execute();
        if (result.getAffectedItemsCount() == 0) return ResponseHelper::notFound(req, "Report template not found");
        return ResponseHelper::created(req, {{"id", result.getAutoIncrementValue()}, {"status", "draft"}});
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
        auto exists = session->sql("SELECT 1 FROM report_template_versions WHERE id=? AND template_id=? LIMIT 1")
            .bind(versionId, templateId).execute();
        if (!exists.fetchOne()) return ResponseHelper::notFound(req, "Report template version not found");
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
