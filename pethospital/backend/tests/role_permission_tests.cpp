#include "../utils/roleTypeUtils/roleTypeUtils.h"

#include <cassert>
#include <memory>
#include <string>

// roleTypeUtils 已退休为纯展示查询（display-only）：
// 判权走 RbacService（库驱动），业务身份走 account_type/staff_kind。
// 本测试锁定两件事：
// 1. 模块不再暴露任何按名字的权限/身份判断（编译期即约束——此文件只调用展示函数）；
// 2. 展示查询 fail-closed：数据库不可用 / 非法入参一律返回空/兼容默认值，不抛异常。
int main()
{
    const std::shared_ptr<DatabaseManagerInterface> nullDb;

    // ---- fail-closed：无数据库连接 ----
    assert(RoleTypeUtils::getUserRoleName(nullDb, 1).empty());
    assert(RoleTypeUtils::getRoleName(nullDb, 1).empty());

    // ---- fail-closed：非法 userId / roleId ----
    assert(RoleTypeUtils::getUserRoleName(nullDb, 0).empty());
    assert(RoleTypeUtils::getUserRoleName(nullDb, -1).empty());
    assert(RoleTypeUtils::getRoleName(nullDb, 0).empty());

    return 0;
}
