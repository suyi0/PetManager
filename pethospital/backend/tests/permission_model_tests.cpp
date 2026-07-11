#include "../utils/permissions/Permissions.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

// 权限目录契约（动态 RBAC 版）：
// 1. 权限 key 目录是代码常量单一真相源；角色→权限映射已入库（position_permissions），
//    其正确性由 seed 等价性测试（rbac_schema_tests）与 RbacService 测试锁定。
// 2. 元权限 rbac:manage：known 但绝不可授予（grant-of-grant 防线的目录层）。
// 3. 未知 key 一律 fail-closed。
namespace
{
bool contains(const std::vector<std::string> &items, const std::string &value)
{
    return std::find(items.begin(), items.end(), value) != items.end();
}
}

int main()
{
    const std::vector<std::string> all = Permissions::allPermissionKeys();
    const std::vector<std::string> grantable = Permissions::grantablePermissionKeys();

    // ---- 目录完整性：全部常量都在 all 里 ----
    const std::vector<std::string> expectedKeys = {
        Permissions::kPortalBoss,
        Permissions::kPortalFinance,
        Permissions::kPortalSuperAdmin,
        Permissions::kPortalPersonnel,
        Permissions::kPortalMedical,
        Permissions::kPortalWarehouse,
        Permissions::kPortalUser,
        Permissions::kSalaryRead,
        Permissions::kSalaryWrite,
        Permissions::kLogsRead,
        Permissions::kMedicalRecordRead,
        Permissions::kMedicalRecordWrite,
        Permissions::kDoctorWorkWrite,
        Permissions::kUserDelete,
        Permissions::kEquityRead,
        Permissions::kEquityWrite,
        Permissions::kStockRead,
        Permissions::kStockWrite,
        Permissions::kStaffRoleWrite,
        Permissions::kAttendanceRead,
        Permissions::kAttendanceManage,
        Permissions::kScopeAll,
        Permissions::kScopeMedicalAssigned,
        Permissions::kRbacManage,
    };
    for (const std::string &key : expectedKeys)
    {
        assert(contains(all, key));
        assert(Permissions::isKnownPermissionKey(key));
    }

    // ---- 元权限不可委派：grantable = all − rbac:manage，仅此一项差异 ----
    assert(!contains(grantable, Permissions::kRbacManage));
    assert(!Permissions::isGrantablePermissionKey(Permissions::kRbacManage));
    assert(grantable.size() == all.size() - 1);
    for (const std::string &key : grantable)
    {
        assert(Permissions::isKnownPermissionKey(key));
        assert(Permissions::isGrantablePermissionKey(key));
    }

    // ---- portal:user 已进目录且可授予（Boss 职位靠它保住用户端入口等价） ----
    assert(Permissions::isGrantablePermissionKey(Permissions::kPortalUser));

    // ---- 未知 key fail-closed ----
    assert(!Permissions::isKnownPermissionKey(""));
    assert(!Permissions::isKnownPermissionKey("portal:hacker"));
    assert(!Permissions::isGrantablePermissionKey("portal:hacker"));

    return 0;
}
