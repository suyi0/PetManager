#include "../utils/roleTypeUtils/roleTypeUtils.h"

#include <cassert>
#include <memory>
#include <string>

// 权限边界测试：RoleTypeUtils 是 orderCommon / reservationCommon / searchCommon
// 及 jwtUtils 权限判定的共同收口点。本测试锁定两件事：
// 1. 角色分类矩阵——谁属于哪个权限组是安全契约，数组被误改立刻挂测试；
// 2. fail-closed——数据库不可用 / userId 非法 / 角色查不到时，一律回退为
//    "无角色"，任何权限谓词都不得放行。
int main()
{
    const std::shared_ptr<DatabaseManagerInterface> nullDb;

    // ---- fail-closed：无数据库连接 ----
    assert(RoleTypeUtils::getUserRoleName(nullDb, 1).empty());
    assert(RoleTypeUtils::getRoleName(nullDb, 1).empty());
    assert(RoleTypeUtils::getRoleId(nullDb, "医生") == 0);
    assert(!RoleTypeUtils::userHasManagementRole(nullDb, 1));
    assert(!RoleTypeUtils::userHasBossRole(nullDb, 1));
    assert(!RoleTypeUtils::userHasPersonnelRole(nullDb, 1));
    assert(!RoleTypeUtils::userHasRole(nullDb, 1, "医生"));

    // ---- fail-closed：非法 userId / roleId ----
    assert(RoleTypeUtils::getUserRoleName(nullDb, 0).empty());
    assert(RoleTypeUtils::getUserRoleName(nullDb, -1).empty());
    assert(RoleTypeUtils::getRoleName(nullDb, 0).empty());

    // ---- 空角色名（用户不存在 / DB 降级的统一回退值）不属于任何权限组 ----
    assert(!RoleTypeUtils::isManagementRole(""));
    assert(!RoleTypeUtils::isBossRole(""));
    assert(!RoleTypeUtils::isPersonnelRole(""));
    assert(!RoleTypeUtils::isMedicalStaffRole(""));
    assert(!RoleTypeUtils::isWarehouseStaffRole(""));
    assert(!RoleTypeUtils::isNormalUserRole(""));

    // ---- Boss（全量数据可见性：order/reservation/search 免 owner 过滤）----
    assert(RoleTypeUtils::isBossRole("总裁"));
    assert(RoleTypeUtils::isBossRole("副总裁"));
    assert(!RoleTypeUtils::isBossRole("超级管理员")); // 管理层 ≠ Boss
    assert(!RoleTypeUtils::isBossRole("财务总监"));
    assert(!RoleTypeUtils::isBossRole("医生"));

    // ---- 管理层（session-version 强制失效的适用范围）----
    assert(RoleTypeUtils::isManagementRole("总裁"));
    assert(RoleTypeUtils::isManagementRole("副总裁"));
    assert(RoleTypeUtils::isManagementRole("财务总监"));
    assert(RoleTypeUtils::isManagementRole("财务经理"));
    assert(RoleTypeUtils::isManagementRole("部门经理"));
    assert(RoleTypeUtils::isManagementRole("超级管理员"));
    assert(!RoleTypeUtils::isManagementRole("人事经理")); // 现行契约：人事不在管理层组
    assert(!RoleTypeUtils::isManagementRole("医生"));
    assert(!RoleTypeUtils::isManagementRole("护士"));
    assert(!RoleTypeUtils::isManagementRole("仓库管理员"));
    assert(!RoleTypeUtils::isManagementRole("普通用户"));

    // ---- 医护（order/reservation 按 doctor_id 过滤的分支）----
    assert(RoleTypeUtils::isMedicalStaffRole("医生"));
    assert(RoleTypeUtils::isMedicalStaffRole("护士"));
    assert(!RoleTypeUtils::isMedicalStaffRole("总裁"));
    assert(!RoleTypeUtils::isMedicalStaffRole("仓库管理员"));

    // ---- 人事 / 仓管 / 普通用户 ----
    assert(RoleTypeUtils::isPersonnelRole("人事经理"));
    assert(!RoleTypeUtils::isPersonnelRole("部门经理"));
    assert(RoleTypeUtils::isWarehouseStaffRole("仓库管理员"));
    assert(!RoleTypeUtils::isWarehouseStaffRole("医生"));
    assert(RoleTypeUtils::isNormalUserRole("普通用户"));
    assert(!RoleTypeUtils::isNormalUserRole("医生"));

    // ---- 精确匹配：不做子串/前缀匹配，防"总裁办"之类的伪角色越权 ----
    assert(!RoleTypeUtils::isBossRole("总裁办"));
    assert(!RoleTypeUtils::isBossRole(" 总裁"));
    assert(!RoleTypeUtils::isBossRole("总"));
    assert(!RoleTypeUtils::isMedicalStaffRole("医生助理"));

    return 0;
}
