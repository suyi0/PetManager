#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef DATABASE_MIGRATIONS_SOURCE_PATH
#error "DATABASE_MIGRATIONS_SOURCE_PATH is required"
#endif
#ifndef DOCTOR_ROUTES_SOURCE_PATH
#error "DOCTOR_ROUTES_SOURCE_PATH is required"
#endif
#ifndef USER_ROUTES_SOURCE_PATH
#error "USER_ROUTES_SOURCE_PATH is required"
#endif
#ifndef ADMIN_ROUTES_SOURCE_PATH
#error "ADMIN_ROUTES_SOURCE_PATH is required"
#endif
#ifndef MEDICAL_DOCUMENT_HANDLER_SOURCE_PATH
#error "MEDICAL_DOCUMENT_HANDLER_SOURCE_PATH is required"
#endif
#ifndef REPORT_TEMPLATE_HANDLER_SOURCE_PATH
#error "REPORT_TEMPLATE_HANDLER_SOURCE_PATH is required"
#endif
#ifndef MAIN_SOURCE_PATH
#error "MAIN_SOURCE_PATH is required"
#endif

namespace
{
std::string readFile(const char *path)
{
    std::ifstream file(path);
    assert(file.is_open());
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void contains(const std::string &source, const std::string &needle)
{
    assert(source.find(needle) != std::string::npos);
}

void notContains(const std::string &source, const std::string &needle)
{
    assert(source.find(needle) == std::string::npos);
}
}

int main()
{
    const std::string migrations = readFile(DATABASE_MIGRATIONS_SOURCE_PATH);
    const std::string doctorRoutes = readFile(DOCTOR_ROUTES_SOURCE_PATH);
    const std::string userRoutes = readFile(USER_ROUTES_SOURCE_PATH);
    const std::string adminRoutes = readFile(ADMIN_ROUTES_SOURCE_PATH);
    const std::string medicalHandler = readFile(MEDICAL_DOCUMENT_HANDLER_SOURCE_PATH);
    const std::string templateHandler = readFile(REPORT_TEMPLATE_HANDLER_SOURCE_PATH);
    const std::string mainSource = readFile(MAIN_SOURCE_PATH);

    for (const char *table : {
             "report_templates", "report_template_versions", "medical_documents",
             "medical_prescription_items", "medical_document_versions", "report_artifacts"})
    {
        contains(migrations, std::string("CREATE TABLE ") + table);
    }
    contains(migrations, "UNIQUE INDEX uq_medical_documents_order");
    contains(migrations, "UNIQUE INDEX uq_medical_document_revision");
    contains(migrations, "UNIQUE INDEX uq_report_artifact_version_template_format");
    contains(migrations, "fk_report_templates_current_version");
    contains(migrations, "v.id=t.current_version_id AND v.template_id=t.id");
    contains(migrations, "seedAllPositionPermissions");
    contains(migrations, "seedAllPermissionTemplateItems");
    contains(migrations, "existing.position_id = p.id AND existing.permission_key = v.permission_key");
    contains(migrations, "existing.template_id = t.id AND existing.permission_key = v.permission_key");
    contains(migrations, "medical-record:amend");
    contains(migrations, "medical-record:void");

    contains(doctorRoutes, "/api/doctors/orders/<int>/medical-document/finalize");
    contains(doctorRoutes, "Permissions::kMedicalRecordFinalize");
    contains(doctorRoutes, "/api/doctors/orders/<int>/medical-document/amendments");
    contains(doctorRoutes, "Permissions::kMedicalRecordAmend");
    contains(doctorRoutes, "/api/doctors/orders/<int>/medical-document/void");
    contains(doctorRoutes, "Permissions::kMedicalRecordVoid");
    contains(doctorRoutes, "/api/doctors/orders/<int>/medical-document/versions");
    contains(doctorRoutes, "/api/doctors/orders/<int>/medical-document/versions/<int>/pdf");
    contains(doctorRoutes, "JwtUtils::isUserAuthorizedForOrder");

    contains(userRoutes, "/api/users/me/medical-documents");
    contains(userRoutes, "/api/users/me/medical-documents/<int>/pdf");
    contains(userRoutes, "isValidUserToken");
    contains(medicalHandler, "md.owner_id=?");
    contains(medicalHandler, "md.status IN ('finalized','amended')");
    contains(medicalHandler, "catch (const mysqlx::Error &)");
    contains(medicalHandler, "const auto previousSnapshot = loadLatestSnapshot");
    contains(medicalHandler, "payload[\"visit\"] = changedPayload[\"visit\"]");
    contains(medicalHandler, "WHERE id=? AND status='draft'");
    contains(medicalHandler, "WHERE id=? AND status IN ('finalized','amended')");

    contains(adminRoutes, "/api/admin/report-templates/<int>/previews");
    contains(adminRoutes, "Permissions::kReportTemplateManage");
    contains(adminRoutes, "Permissions::kReportTemplatePublish");
    contains(templateHandler, "std::transform(normalized.begin()");
    contains(templateHandler, "\"script\", \"meta\", \"base\", \"iframe\"");
    contains(templateHandler, "sampleMedicalDocumentPayload");
    contains(templateHandler, "FOR UPDATE");
    contains(templateHandler, "不能发布");

    contains(mainSource, "WebSocketServer::instance().gracefulShutdown();");
    notContains(mainSource, "if (!WebSocketServer::instance().isServerStopped())");

    return 0;
}
