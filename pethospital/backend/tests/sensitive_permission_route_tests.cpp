#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef ADMIN_ROUTES_SOURCE_PATH
#error "ADMIN_ROUTES_SOURCE_PATH is required"
#endif

#ifndef FINANCE_ROUTES_SOURCE_PATH
#error "FINANCE_ROUTES_SOURCE_PATH is required"
#endif

#ifndef SET_ROUTES_SOURCE_PATH
#error "SET_ROUTES_SOURCE_PATH is required"
#endif

#ifndef BOSS_ROUTES_SOURCE_PATH
#error "BOSS_ROUTES_SOURCE_PATH is required"
#endif

#ifndef WAREHOUSE_ROUTES_SOURCE_PATH
#error "WAREHOUSE_ROUTES_SOURCE_PATH is required"
#endif

#ifndef PERSONNEL_ROUTES_SOURCE_PATH
#error "PERSONNEL_ROUTES_SOURCE_PATH is required"
#endif

#ifndef DOCTOR_ROUTES_SOURCE_PATH
#error "DOCTOR_ROUTES_SOURCE_PATH is required"
#endif

#ifndef OPERATION_LOGGER_SOURCE_PATH
#error "OPERATION_LOGGER_SOURCE_PATH is required"
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

std::size_t countOccurrences(const std::string &source, const std::string &needle)
{
    std::size_t count = 0;
    std::size_t pos = source.find(needle);
    while (pos != std::string::npos)
    {
        ++count;
        pos = source.find(needle, pos + needle.size());
    }
    return count;
}

std::string sectionBetween(const std::string &source, const std::string &startNeedle, const std::string &endNeedle)
{
    const std::size_t start = source.find(startNeedle);
    assert(start != std::string::npos);

    const std::size_t end = source.find(endNeedle, start + startNeedle.size());
    assert(end != std::string::npos);

    return source.substr(start, end - start);
}

void assertContains(const std::string &source, const std::string &needle)
{
    assert(source.find(needle) != std::string::npos);
}

void assertNotContains(const std::string &source, const std::string &needle)
{
    assert(source.find(needle) == std::string::npos);
}

void assertSensitiveAudit(const std::string &section, const std::string &permissionKey)
{
    assertContains(section, "OperationLogger::FinishSensitiveRoute(");
    assertContains(section, permissionKey);
    assertNotContains(section, "OperationLogger::FinishLoggedRoute(dbManager, req, res");
}
}

int main()
{
    const std::string adminRoutes = readFile(ADMIN_ROUTES_SOURCE_PATH);
    const std::string financeRoutes = readFile(FINANCE_ROUTES_SOURCE_PATH);
    const std::string setRoutes = readFile(SET_ROUTES_SOURCE_PATH);
    const std::string bossRoutes = readFile(BOSS_ROUTES_SOURCE_PATH);
    const std::string warehouseRoutes = readFile(WAREHOUSE_ROUTES_SOURCE_PATH);
    const std::string personnelRoutes = readFile(PERSONNEL_ROUTES_SOURCE_PATH);
    const std::string doctorRoutes = readFile(DOCTOR_ROUTES_SOURCE_PATH);
    const std::string operationLogger = readFile(OPERATION_LOGGER_SOURCE_PATH);

    const std::string adminHomeSection = sectionBetween(
        adminRoutes,
        "CROW_ROUTE(app, \"/api/admins/home-data\")",
        "CROW_WEBSOCKET_ROUTE(app, \"/realtime/admins/home-data\")");
    const std::string adminUsersSection = sectionBetween(
        adminRoutes,
        "CROW_ROUTE(app, \"/api/admins/users\")",
        "CROW_ROUTE(app, \"/api/admins/users/search\")");
    const std::string adminUserSearchSection = sectionBetween(
        adminRoutes,
        "CROW_ROUTE(app, \"/api/admins/users/search\")",
        "CROW_ROUTE(app, \"/api/admins/online-doctors/search\")");
    const std::string userDeletionSection = sectionBetween(
        adminRoutes,
        "CROW_ROUTE(app, \"/api/admins/user-deletions\")",
        "CROW_ROUTE(app, \"/api/admins/doctor-work-time-changes\")");
    const std::string doctorWorkTimeSection = sectionBetween(
        adminRoutes,
        "CROW_ROUTE(app, \"/api/admins/doctor-work-time-changes\")",
        "CROW_ROUTE(app, \"/api/admins/doctor-work-status-changes\")");
    const std::string doctorWorkStatusSection = sectionBetween(
        adminRoutes,
        "CROW_ROUTE(app, \"/api/admins/doctor-work-status-changes\")",
        "CROW_ROUTE(app, \"/api/admins/logs\")");
    const std::string logsSection = sectionBetween(
        adminRoutes,
        "CROW_ROUTE(app, \"/api/admins/logs\")",
        "CROW_ROUTE(app, \"/api/admins/logs/search\")");
    const std::string logsSearchSection = sectionBetween(
        adminRoutes,
        "CROW_ROUTE(app, \"/api/admins/logs/search\")",
        "CROW_ROUTE(app, \"/api/admins/order-records\")");
    const std::string adminOrderRecordsSection = sectionBetween(
        adminRoutes,
        "CROW_ROUTE(app, \"/api/admins/order-records\")",
        "routes_setup = true;");

    assertNotContains(adminHomeSection, "isValidPermissionToken(");
    assertNotContains(adminUsersSection, "isValidPermissionToken(");
    assertNotContains(adminUserSearchSection, "isValidPermissionToken(");
    assertContains(userDeletionSection, "isValidPermissionToken(req, res, dbManager, Permissions::kUserDelete)");
    assertContains(logsSection, "isValidPermissionToken(req, res, dbManager, Permissions::kLogsRead)");
    assertContains(logsSearchSection, "isValidPermissionToken(req, res, dbManager, Permissions::kLogsRead)");
    assertContains(adminOrderRecordsSection, "isValidPermissionToken(req, res, dbManager, Permissions::kMedicalRecordRead)");
    assertContains(doctorWorkTimeSection, "isValidPermissionToken(req, res, dbManager, Permissions::kDoctorWorkWrite)");
    assertContains(doctorWorkStatusSection, "isValidPermissionToken(req, res, dbManager, Permissions::kDoctorWorkWrite)");
    assertSensitiveAudit(userDeletionSection, "Permissions::kUserDelete");
    assertSensitiveAudit(doctorWorkTimeSection, "Permissions::kDoctorWorkWrite");
    assertSensitiveAudit(doctorWorkStatusSection, "Permissions::kDoctorWorkWrite");
    assertSensitiveAudit(logsSection, "Permissions::kLogsRead");
    assertSensitiveAudit(logsSearchSection, "Permissions::kLogsRead");
    assertSensitiveAudit(adminOrderRecordsSection, "Permissions::kMedicalRecordRead");
    assertNotContains(adminOrderRecordsSection, "isValidUserToken(");
    assertNotContains(doctorWorkTimeSection, "isValidSuperAdminPortalToken(");
    assertNotContains(doctorWorkStatusSection, "isValidSuperAdminPortalToken(");

    const std::string financeHomeSection = sectionBetween(
        financeRoutes,
        "CROW_ROUTE(app, \"/api/finance/home-data\")",
        "CROW_WEBSOCKET_ROUTE(app, \"/realtime/finance/home-data\")");
    const std::string salaryWriteSection = sectionBetween(
        financeRoutes,
        "CROW_ROUTE(app, \"/api/finance/employee-salaries/<int>\")",
        "CROW_ROUTE(app, \"/api/finance/employees/<int>/salary-profile\")");
    const std::string salaryProfileWriteSection = sectionBetween(
        financeRoutes,
        "CROW_ROUTE(app, \"/api/finance/employees/<int>/salary-profile\")",
        "CROW_ROUTE(app, \"/api/finance/payroll-periods/current/employees/<int>/first-review\")");
    const std::string salaryFirstReviewSection = sectionBetween(
        financeRoutes,
        "CROW_ROUTE(app, \"/api/finance/payroll-periods/current/employees/<int>/first-review\")",
        "CROW_ROUTE(app, \"/api/finance/salary-summaries/<int>\")");
    const std::string salarySummarySection = sectionBetween(
        financeRoutes,
        "CROW_ROUTE(app, \"/api/finance/salary-summaries/<int>\")",
        "CROW_ROUTE(app, \"/api/finance/salary-employees/search\")");
    const std::string salarySearchSection = sectionBetween(
        financeRoutes,
        "CROW_ROUTE(app, \"/api/finance/salary-employees/search\")",
        "CROW_ROUTE(app, \"/api/finance/payroll-periods/current/submit-review\")");
    const std::string salarySubmitReviewSection = sectionBetween(
        financeRoutes,
        "CROW_ROUTE(app, \"/api/finance/payroll-periods/current/submit-review\")",
        "CROW_ROUTE(app, \"/api/finance/payroll-periods/current/lock\")");
    const std::string salaryLockSection = sectionBetween(
        financeRoutes,
        "CROW_ROUTE(app, \"/api/finance/payroll-periods/current/lock\")",
        "CROW_ROUTE(app, \"/api/finance/payroll-periods/current/revisions\")");
    const std::string salaryRevisionSection = sectionBetween(
        financeRoutes,
        "CROW_ROUTE(app, \"/api/finance/payroll-periods/current/revisions\")",
        "CROW_ROUTE(app, \"/api/finance/salary-records/<int>\")");
    const std::string salaryRecordSection = sectionBetween(
        financeRoutes,
        "CROW_ROUTE(app, \"/api/finance/salary-records/<int>\")",
        "CROW_ROUTE(app, \"/api/finance/expenses\")");

    assertNotContains(financeHomeSection, "isValidPermissionToken(");
    assertContains(setRoutes, "financeRoutes::setupFinanceRoutes(*app_ptr_, DatabaseManager::getInstance())");
    assertContains(salaryWriteSection, "isValidPermissionToken(req, res, dbManager, Permissions::kSalaryWrite)");
    assertContains(salaryProfileWriteSection, "isWrite ? Permissions::kSalaryWrite : Permissions::kSalaryRead");
    assertContains(salaryFirstReviewSection, "isValidPermissionToken(req, res, dbManager, Permissions::kSalaryReview)");
    assertContains(salarySummarySection, "isValidPermissionToken(req, res, dbManager, Permissions::kSalaryRead)");
    assertContains(salarySearchSection, "isValidPermissionToken(req, res, dbManager, Permissions::kSalaryRead)");
    assertContains(salarySubmitReviewSection, "isValidPermissionToken(req, res, dbManager, Permissions::kSalaryReview)");
    assertContains(salaryLockSection, "isValidPermissionToken(req, res, dbManager, Permissions::kSalaryLock)");
    assertContains(salaryRevisionSection, "isValidPermissionToken(req, res, dbManager, Permissions::kSalaryWrite)");
    assertContains(salaryRecordSection, "isValidPermissionToken(req, res, dbManager, Permissions::kSalaryRead)");
    assertSensitiveAudit(salaryWriteSection, "Permissions::kSalaryWrite");
    assertContains(salaryProfileWriteSection, "FinishSensitiveRoute");
    assertSensitiveAudit(salaryFirstReviewSection, "Permissions::kSalaryReview");
    assertSensitiveAudit(salarySummarySection, "Permissions::kSalaryRead");
    assertSensitiveAudit(salarySearchSection, "Permissions::kSalaryRead");
    assertSensitiveAudit(salarySubmitReviewSection, "Permissions::kSalaryReview");
    assertSensitiveAudit(salaryLockSection, "Permissions::kSalaryLock");
    assertSensitiveAudit(salaryRevisionSection, "Permissions::kSalaryWrite");
    assertSensitiveAudit(salaryRecordSection, "Permissions::kSalaryRead");

    const std::string totalStockAllocationSection = sectionBetween(
        bossRoutes,
        "CROW_ROUTE(app, \"/api/bosses/total-stock-allocations\")",
        "CROW_ROUTE(app, \"/api/bosses/stock-allocations\")");
    const std::string stockAllocationSection = sectionBetween(
        bossRoutes,
        "CROW_ROUTE(app, \"/api/bosses/stock-allocations\")",
        "CROW_ROUTE(app, \"/api/bosses/stock-changes\")");
    const std::string stockChangeSection = sectionBetween(
        bossRoutes,
        "CROW_ROUTE(app, \"/api/bosses/stock-changes\")",
        "CROW_ROUTE(app, \"/api/bosses/stocks\")");
    const std::string stockReadSection = sectionBetween(
        bossRoutes,
        "CROW_ROUTE(app, \"/api/bosses/stocks\")",
        "});\n}");

    assertContains(totalStockAllocationSection, "isValidPermissionToken(req, res, dbManager, Permissions::kEquityWrite)");
    assertContains(stockAllocationSection, "isValidPermissionToken(req, res, dbManager, Permissions::kEquityWrite)");
    assertContains(stockChangeSection, "isValidPermissionToken(req, res, dbManager, Permissions::kEquityWrite)");
    assertContains(stockReadSection, "isValidPermissionToken(req, res, dbManager, Permissions::kEquityRead)");
    assertSensitiveAudit(totalStockAllocationSection, "Permissions::kEquityWrite");
    assertSensitiveAudit(stockAllocationSection, "Permissions::kEquityWrite");
    assertSensitiveAudit(stockChangeSection, "Permissions::kEquityWrite");
    assertSensitiveAudit(stockReadSection, "Permissions::kEquityRead");

    const std::string warehouseItemsSection = sectionBetween(
        warehouseRoutes,
        "CROW_ROUTE(app, \"/api/warehouse-managers/items\")",
        "CROW_ROUTE(app, \"/api/warehouse-managers/items/search\")");
    const std::string warehouseSearchSection = sectionBetween(
        warehouseRoutes,
        "CROW_ROUTE(app, \"/api/warehouse-managers/items/search\")",
        "CROW_ROUTE(app, \"/api/warehouse-managers/items/<string>/<string>\")");
    const std::string warehouseFilteredReadSection = sectionBetween(
        warehouseRoutes,
        "CROW_ROUTE(app, \"/api/warehouse-managers/items/<string>/<string>\")",
        "CROW_ROUTE(app, \"/api/warehouse-managers/items/<int>\")");
    const std::string warehouseUpdateSection = sectionBetween(
        warehouseRoutes,
        "CROW_ROUTE(app, \"/api/warehouse-managers/items/<int>\")",
        "CROW_ROUTE(app, \"/api/warehouse-managers/item-deletions\")");
    const std::string warehouseDeleteSection = sectionBetween(
        warehouseRoutes,
        "CROW_ROUTE(app, \"/api/warehouse-managers/item-deletions\")",
        "routes_setup = true;");

    assertContains(warehouseItemsSection, "Permissions::kStockRead");
    assertContains(warehouseItemsSection, "Permissions::kStockWrite");
    assertContains(warehouseItemsSection, "isValidPermissionToken(req, res, dbManager, permissionKey)");
    assertContains(warehouseItemsSection, "OperationLogger::FinishSensitiveRoute(");
    assertContains(warehouseItemsSection, "if (isUpload)");
    assertContains(warehouseSearchSection, "isValidPermissionToken(req, res, dbManager, Permissions::kStockRead)");
    assertContains(warehouseFilteredReadSection, "isValidPermissionToken(req, res, dbManager, Permissions::kStockRead)");
    assertContains(warehouseUpdateSection, "isValidPermissionToken(req, res, dbManager, Permissions::kStockWrite)");
    assertContains(warehouseDeleteSection, "isValidPermissionToken(req, res, dbManager, Permissions::kStockWrite)");
    assertSensitiveAudit(warehouseUpdateSection, "Permissions::kStockWrite");
    assertSensitiveAudit(warehouseDeleteSection, "Permissions::kStockWrite");
    assertNotContains(warehouseItemsSection, "isValidWarehouseStaffToken(");
    assertNotContains(warehouseSearchSection, "isValidWarehouseStaffToken(");
    assertNotContains(warehouseFilteredReadSection, "isValidWarehouseStaffToken(");
    assertNotContains(warehouseUpdateSection, "isValidWarehouseStaffToken(");
    assertNotContains(warehouseDeleteSection, "isValidWarehouseStaffToken(");

    const std::string doctorAssignmentSection = sectionBetween(
        personnelRoutes,
        "CROW_ROUTE(app, \"/api/personnel/doctor-assignments\")",
        "CROW_ROUTE(app, \"/api/personnel/doctor-removals\")");
    const std::string doctorRemovalSection = sectionBetween(
        personnelRoutes,
        "CROW_ROUTE(app, \"/api/personnel/doctor-removals\")",
        "CROW_ROUTE(app, \"/api/personnel/warehouse-manager-assignments\")");
    const std::string warehouseManagerAssignmentSection = sectionBetween(
        personnelRoutes,
        "CROW_ROUTE(app, \"/api/personnel/warehouse-manager-assignments\")",
        "CROW_ROUTE(app, \"/api/personnel/warehouse-manager-removals\")");
    const std::string warehouseManagerRemovalSection = sectionBetween(
        personnelRoutes,
        "CROW_ROUTE(app, \"/api/personnel/warehouse-manager-removals\")",
        "routes_setup = true;");

    // 派职（委任医生/仓管）= 把人派进含权限职位 = 授权，须 rbac:manage（超管独占），
    // 不能用可委派的 staff-role:write（否则 personnel 成二级授权旁路，DESIGN §8）。
    assertContains(doctorAssignmentSection, "isValidPermissionToken(req, res, dbManager, Permissions::kRbacManage)");
    assertContains(warehouseManagerAssignmentSection, "isValidPermissionToken(req, res, dbManager, Permissions::kRbacManage)");
    assertSensitiveAudit(doctorAssignmentSection, "Permissions::kRbacManage");
    assertSensitiveAudit(warehouseManagerAssignmentSection, "Permissions::kRbacManage");
    // 摘除（医生/仓管降级为普通用户）= 收权，安全，保留 staff-role:write。
    assertContains(doctorRemovalSection, "isValidPermissionToken(req, res, dbManager, Permissions::kStaffRoleWrite)");
    assertContains(warehouseManagerRemovalSection, "isValidPermissionToken(req, res, dbManager, Permissions::kStaffRoleWrite)");
    assertSensitiveAudit(doctorRemovalSection, "Permissions::kStaffRoleWrite");
    assertSensitiveAudit(warehouseManagerRemovalSection, "Permissions::kStaffRoleWrite");
    assertNotContains(doctorAssignmentSection, "isValidPersonnelToken(");
    assertNotContains(doctorRemovalSection, "isValidPersonnelToken(");
    assertNotContains(warehouseManagerAssignmentSection, "isValidPersonnelToken(");
    assertNotContains(warehouseManagerRemovalSection, "isValidPersonnelToken(");

    const std::string doctorOrderRecordSection = sectionBetween(
        doctorRoutes,
        "CROW_ROUTE(app, \"/api/doctors/order-records\")",
        "CROW_ROUTE(app, \"/api/doctors/order-summaries\")");

    assertContains(doctorOrderRecordSection, "isValidPermissionToken(req, res, dbManager, Permissions::kMedicalRecordWrite)");
    assertSensitiveAudit(doctorOrderRecordSection, "Permissions::kMedicalRecordWrite");
    assertNotContains(doctorOrderRecordSection, "isValidMedicalStaffToken(");

    assertContains(operationLogger, "FinishSensitiveRoute");
    assertContains(operationLogger, "\"auditType\"");
    assertContains(operationLogger, "\"sensitive_permission\"");
    assertContains(operationLogger, "\"permissionKey\"");
    assertContains(operationLogger, "sensitive:");

    return 0;
}
