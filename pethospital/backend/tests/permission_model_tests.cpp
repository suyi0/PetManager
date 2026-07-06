#include "../utils/permissions/Permissions.h"

#include <cassert>
#include <string>

namespace
{
void assertNoPortalOrSensitivePermission(const std::string &roleName)
{
    assert(!Permissions::roleHasPermission(roleName, Permissions::kPortalBoss));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kPortalFinance));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kPortalSuperAdmin));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kPortalPersonnel));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kPortalMedical));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kPortalWarehouse));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kSalaryRead));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kSalaryWrite));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kLogsRead));
    assert(!Permissions::roleHasPermission(roleName, "medical-record:read"));
    assert(!Permissions::roleHasPermission(roleName, "medical-record:write"));
    assert(!Permissions::roleHasPermission(roleName, "doctor-work:write"));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kUserDelete));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kEquityRead));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kEquityWrite));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kStockRead));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kStockWrite));
    assert(!Permissions::roleHasPermission(roleName, "staff-role:write"));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kScopeAll));
    assert(!Permissions::roleHasPermission(roleName, Permissions::kScopeMedicalAssigned));
}
}

int main()
{
    // Boss package: current product semantics allow cross-management visibility.
    assert(Permissions::roleHasPermission("总裁", Permissions::kPortalBoss));
    assert(Permissions::roleHasPermission("总裁", Permissions::kPortalFinance));
    assert(Permissions::roleHasPermission("总裁", Permissions::kPortalSuperAdmin));
    assert(Permissions::roleHasPermission("总裁", Permissions::kSalaryRead));
    assert(Permissions::roleHasPermission("总裁", Permissions::kSalaryWrite));
    assert(Permissions::roleHasPermission("总裁", Permissions::kLogsRead));
    assert(Permissions::roleHasPermission("总裁", "medical-record:read"));
    assert(Permissions::roleHasPermission("总裁", "medical-record:write"));
    assert(Permissions::roleHasPermission("总裁", "doctor-work:write"));
    assert(Permissions::roleHasPermission("总裁", Permissions::kUserDelete));
    assert(Permissions::roleHasPermission("总裁", Permissions::kEquityRead));
    assert(Permissions::roleHasPermission("总裁", Permissions::kEquityWrite));
    assert(Permissions::roleHasPermission("总裁", "staff-role:write"));
    assert(Permissions::roleHasPermission("总裁", Permissions::kScopeAll));
    assert(!Permissions::roleHasPermission("总裁", Permissions::kScopeMedicalAssigned));

    assert(Permissions::roleHasPermission("副总裁", Permissions::kPortalBoss));
    assert(Permissions::roleHasPermission("副总裁", Permissions::kPortalFinance));
    assert(Permissions::roleHasPermission("副总裁", Permissions::kPortalSuperAdmin));
    assert(Permissions::roleHasPermission("副总裁", "medical-record:read"));
    assert(Permissions::roleHasPermission("副总裁", "medical-record:write"));
    assert(Permissions::roleHasPermission("副总裁", "doctor-work:write"));
    assert(Permissions::roleHasPermission("副总裁", Permissions::kEquityRead));
    assert(Permissions::roleHasPermission("副总裁", Permissions::kEquityWrite));
    assert(Permissions::roleHasPermission("副总裁", "staff-role:write"));

    // Finance package: salary access, but no boss or super-admin portal privileges.
    assert(Permissions::roleHasPermission("财务总监", Permissions::kPortalFinance));
    assert(Permissions::roleHasPermission("财务总监", Permissions::kSalaryRead));
    assert(Permissions::roleHasPermission("财务总监", Permissions::kSalaryWrite));
    assert(!Permissions::roleHasPermission("财务总监", Permissions::kPortalBoss));
    assert(!Permissions::roleHasPermission("财务总监", Permissions::kPortalSuperAdmin));
    assert(!Permissions::roleHasPermission("财务总监", Permissions::kLogsRead));
    assert(!Permissions::roleHasPermission("财务总监", "medical-record:read"));
    assert(!Permissions::roleHasPermission("财务总监", "medical-record:write"));
    assert(!Permissions::roleHasPermission("财务总监", "doctor-work:write"));
    assert(!Permissions::roleHasPermission("财务总监", Permissions::kUserDelete));
    assert(!Permissions::roleHasPermission("财务总监", Permissions::kEquityRead));
    assert(!Permissions::roleHasPermission("财务总监", Permissions::kEquityWrite));
    assert(!Permissions::roleHasPermission("财务总监", "staff-role:write"));

    assert(Permissions::roleHasPermission("财务经理", Permissions::kPortalFinance));
    assert(Permissions::roleHasPermission("财务经理", Permissions::kSalaryRead));
    assert(Permissions::roleHasPermission("财务经理", Permissions::kSalaryWrite));
    assert(!Permissions::roleHasPermission("财务经理", Permissions::kPortalBoss));
    assert(!Permissions::roleHasPermission("财务经理", Permissions::kPortalSuperAdmin));

    // Super-admin package: user/log administration, but no salary or boss portal access.
    assert(Permissions::roleHasPermission("部门经理", Permissions::kPortalSuperAdmin));
    assert(Permissions::roleHasPermission("部门经理", Permissions::kLogsRead));
    assert(Permissions::roleHasPermission("部门经理", "medical-record:read"));
    assert(!Permissions::roleHasPermission("部门经理", "medical-record:write"));
    assert(Permissions::roleHasPermission("部门经理", "doctor-work:write"));
    assert(Permissions::roleHasPermission("部门经理", Permissions::kUserDelete));
    assert(!Permissions::roleHasPermission("部门经理", Permissions::kPortalFinance));
    assert(!Permissions::roleHasPermission("部门经理", Permissions::kPortalBoss));
    assert(!Permissions::roleHasPermission("部门经理", Permissions::kSalaryWrite));
    assert(!Permissions::roleHasPermission("部门经理", Permissions::kEquityRead));
    assert(!Permissions::roleHasPermission("部门经理", Permissions::kEquityWrite));

    assert(Permissions::roleHasPermission("超级管理员", Permissions::kPortalSuperAdmin));
    assert(Permissions::roleHasPermission("超级管理员", Permissions::kLogsRead));
    assert(Permissions::roleHasPermission("超级管理员", "medical-record:read"));
    assert(!Permissions::roleHasPermission("超级管理员", "medical-record:write"));
    assert(Permissions::roleHasPermission("超级管理员", "doctor-work:write"));
    assert(Permissions::roleHasPermission("超级管理员", Permissions::kUserDelete));
    assert(!Permissions::roleHasPermission("超级管理员", Permissions::kPortalFinance));
    assert(!Permissions::roleHasPermission("超级管理员", Permissions::kPortalBoss));

    // Other current role packages stay narrow.
    assert(Permissions::roleHasPermission("人事经理", Permissions::kPortalPersonnel));
    assert(Permissions::roleHasPermission("人事经理", "staff-role:write"));
    assert(Permissions::roleHasPermission("医生", Permissions::kPortalMedical));
    assert(Permissions::roleHasPermission("护士", Permissions::kPortalMedical));
    assert(Permissions::roleHasPermission("医生", "medical-record:read"));
    assert(Permissions::roleHasPermission("护士", "medical-record:read"));
    assert(Permissions::roleHasPermission("医生", "medical-record:write"));
    assert(Permissions::roleHasPermission("护士", "medical-record:write"));
    assert(!Permissions::roleHasPermission("医生", "doctor-work:write"));
    assert(!Permissions::roleHasPermission("护士", "doctor-work:write"));
    assert(Permissions::roleHasPermission("医生", Permissions::kScopeMedicalAssigned));
    assert(Permissions::roleHasPermission("护士", Permissions::kScopeMedicalAssigned));
    assert(!Permissions::roleHasPermission("医生", Permissions::kScopeAll));
    assert(Permissions::roleHasPermission("仓库管理员", Permissions::kPortalWarehouse));
    assert(Permissions::roleHasPermission("仓库管理员", Permissions::kStockRead));
    assert(Permissions::roleHasPermission("仓库管理员", Permissions::kStockWrite));

    assertNoPortalOrSensitivePermission("普通用户");
    assertNoPortalOrSensitivePermission("");
    assertNoPortalOrSensitivePermission("总裁办");

    return 0;
}
