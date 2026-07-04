#include "DataScope.h"
#include "../permissions/Permissions.h"

namespace DataScope
{
Scope resolveForRole(const std::string &roleName, int userId)
{
    if (Permissions::roleHasPermission(roleName, Permissions::kScopeAll))
    {
        return {Kind::All, userId};
    }

    if (Permissions::roleHasPermission(roleName, Permissions::kScopeMedicalAssigned))
    {
        return {Kind::MedicalAssigned, userId};
    }

    return {Kind::Owner, userId};
}

bool bindsUserId(const Scope &scope)
{
    return scope.kind != Kind::All;
}
}
