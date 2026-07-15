#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef EMPLOYMENT_ASSIGNMENT_SERVICE_SOURCE_PATH
#error "EMPLOYMENT_ASSIGNMENT_SERVICE_SOURCE_PATH is required"
#endif
#ifndef POSITION_PERMISSION_SERVICE_SOURCE_PATH
#error "POSITION_PERMISSION_SERVICE_SOURCE_PATH is required"
#endif
#ifndef PERSONNEL_HANDLER_SOURCE_PATH
#error "PERSONNEL_HANDLER_SOURCE_PATH is required"
#endif
#ifndef PERSONNEL_ROUTES_SOURCE_PATH
#error "PERSONNEL_ROUTES_SOURCE_PATH is required"
#endif
#ifndef PERSONNEL_ACCESS_SOURCE_PATH
#error "PERSONNEL_ACCESS_SOURCE_PATH is required"
#endif
#ifndef EMPLOYMENT_OUTBOX_SOURCE_PATH
#error "EMPLOYMENT_OUTBOX_SOURCE_PATH is required"
#endif
#ifndef DATABASE_MIGRATIONS_SOURCE_PATH
#error "DATABASE_MIGRATIONS_SOURCE_PATH is required"
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

void contains(const std::string &source, const std::string &value)
{
    assert(source.find(value) != std::string::npos);
}

void notContains(const std::string &source, const std::string &value)
{
    assert(source.find(value) == std::string::npos);
}
}

int main()
{
    const std::string service = readFile(EMPLOYMENT_ASSIGNMENT_SERVICE_SOURCE_PATH);
    const std::string positionService = readFile(POSITION_PERMISSION_SERVICE_SOURCE_PATH);
    const std::string personnelHandler = readFile(PERSONNEL_HANDLER_SOURCE_PATH);
    const std::string personnelRoutes = readFile(PERSONNEL_ROUTES_SOURCE_PATH);
    const std::string personnelAccess = readFile(PERSONNEL_ACCESS_SOURCE_PATH);
    const std::string outbox = readFile(EMPLOYMENT_OUTBOX_SOURCE_PATH);
    const std::string migrations = readFile(DATABASE_MIGRATIONS_SOURCE_PATH);

    // B2/B6: 职位权限事务内原始行 floor
    contains(positionService, "FOR UPDATE");
    contains(positionService, "SELECT permission_key FROM position_permissions WHERE position_id = ?");
    notContains(positionService, "RbacService::loadPermissionsForPosition");

    // B6: assignment gate = max(stored policy, raw-key floor)
    contains(service, "loadRawPermissionKeysUnderLock");
    contains(service, "effectivePolicyGate");
    contains(service, "requiredAssignmentPolicy");
    contains(service, "maxAssignmentPolicy");

    // B3/B5/B10: employment CAS affected==1
    contains(service, "FROM users WHERE id = ? LIMIT 1 FOR UPDATE");
    contains(service, "FROM employment WHERE user_id = ? LIMIT 1 FOR UPDATE");
    contains(service, "INSERT INTO employment_assignment");
    contains(service, "INSERT INTO employment_event_outbox");
    contains(service, "getAffectedItemsCount() != 1");
    contains(service, "STALE_VERSION");
    contains(service, "casBumpEmploymentRowVersion");

    // B4
    contains(service, "Action::Offboard");
    contains(service, "ApprovalRequired");
    contains(service, "SELF_ASSIGNMENT");

    // B7/B16: action 必须显式；精确权限；禁止字段推断
    contains(personnelRoutes, "PersonnelAccess::parseAssignmentAction");
    contains(personnelRoutes, "PersonnelAccess::canPerformAssignmentAction");
    contains(personnelRoutes, "action 必填");
    contains(personnelRoutes, "regularize");
    notContains(personnelRoutes, "expected > 0 ? \"transfer\" : \"onboard\"");
    notContains(personnelRoutes, "resolved = \"offboard\"");
    contains(personnelHandler, "action 必填");
    notContains(personnelHandler, "action == \"onboard\" || action.empty()");
    contains(personnelAccess, "kEmploymentOnboard");
    contains(personnelAccess, "kEmploymentAssign");
    contains(personnelAccess, "kEmploymentOffboard");
    contains(personnelAccess, "operatorHoldsPersonnelDomainPosition");
    contains(personnelAccess, "staff_kind = 'personnel'");
    contains(personnelAccess, "business_domain");
    contains(personnelAccess, "kStaffRoleWrite");

    // B8: staff-role:write 仅人事域
    contains(personnelAccess, "operatorHoldsPersonnelDomainPosition");
    notContains(personnelRoutes, "hasPersonnelEmploymentAccess");

    // B9 seed 纪律
    notContains(migrations, "UNION ALL SELECT 'president', 'employment:onboard'");
    notContains(migrations, "UNION ALL SELECT 'vice-president', 'employment:assign'");
    notContains(migrations, "UNION ALL SELECT 'Boss', 'employment:onboard'");
    notContains(migrations, "UNION ALL SELECT 'Boss', 'employment:offboard'");
    contains(migrations, "UNION ALL SELECT 'president', 'employment-assignment:approve'");
    contains(migrations, "UNION ALL SELECT 'Boss', 'employment-assignment:approve'");
    contains(migrations, "staff_kind = 'personnel' OR COALESCE(d.business_domain, '') = 'personnel'");
    contains(migrations, "staff-role:write");
    contains(migrations, "salary-profile:activate");
    notContains(migrations, "UNION ALL SELECT 'super-admin', 'salary-profile:activate'");
    notContains(migrations, "UNION ALL SELECT 'super-admin', 'compensation:approve'");

    // B17: 迁移安全下限用 catalog，无硬编码 permission_key IN 安全清单
    contains(migrations, "Permissions::requiredAssignmentPolicy");
    contains(migrations, "rawKeys");
    notContains(migrations, "pp.permission_key IN (");
    // B18: 完整消费 positions 后再嵌套查询
    contains(migrations, "std::vector<PositionPolicyRow> positionRows");
    contains(migrations, "positions SqlResult 已离开作用域");

    // B11 outbox 租约
    contains(outbox, "locked_at");
    contains(outbox, "lease_expired");
    contains(outbox, "kProcessingLeaseSeconds");
    contains(outbox, "status = 'processing'");
    contains(migrations, "locked_at DATETIME NULL");

    // B12 受限 customer fail-closed
    contains(personnelHandler, "AND 1 = 0");
    contains(personnelHandler, "u.account_type = 'staff' AND pos.department_id IN");
    notContains(personnelHandler, "u.account_type = 'customer' OR pos.department_id");

    notContains(personnelRoutes, "doctor-assignments");
    notContains(personnelHandler, "createDoctor");

    return 0;
}
