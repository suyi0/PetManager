#include "DataScope.h"
#include "../../services/rbac/RbacService.h"
#include "../permissions/Permissions.h"

namespace DataScope
{
Scope resolveForUser(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId)
{
    // 一次加载判两把 key；加载失败（用户不存在/客户无职位）落到 Owner——只看自己，fail-closed。
    const auto access = RbacService::loadUserAccess(dbManager, userId);
    if (access)
    {
        if (RbacService::accessHasPermission(*access, Permissions::kScopeAll))
        {
            return {Kind::All, userId};
        }
        if (RbacService::accessHasPermission(*access, Permissions::kScopeMedicalAssigned))
        {
            return {Kind::MedicalAssigned, userId};
        }
    }

    return {Kind::Owner, userId};
}

bool bindsUserId(const Scope &scope)
{
    return scope.kind != Kind::All;
}
}
