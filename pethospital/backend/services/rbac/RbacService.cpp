#include "RbacService.h"

#include "../../utils/permissions/Permissions.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <set>

namespace RbacService
{
namespace
{
bool containsPermission(const std::vector<std::string> &permissions, const std::string &permissionKey)
{
    return std::find(permissions.begin(), permissions.end(), permissionKey) != permissions.end();
}

bool isSuperAdminSystemPosition(const std::string &systemKey)
{
    return systemKey == "super-admin";
}
}

std::vector<std::string> loadPermissionsForPosition(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId)
{
    std::vector<std::string> permissions;
    if (!dbManager || !dbManager->getSession() || positionId <= 0)
    {
        return permissions;
    }

    try
    {
        mysqlx::SqlResult result = dbManager->getSession()
                                      ->sql("SELECT permission_key "
                                            "FROM position_permissions "
                                            "WHERE position_id = ?")
                                      .bind(positionId)
                                      .execute();

        for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
        {
            if (!row[0].isNull())
            {
                const std::string permissionKey = row[0].get<std::string>();
                if (Permissions::isKnownPermissionKey(permissionKey))
                {
                    permissions.push_back(permissionKey);
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error loading permissions for position " << positionId << ": " << e.what() << std::endl;
        permissions.clear();
    }

    return permissions;
}

bool positionHasPermission(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId,
    const std::string &permissionKey)
{
    if (!Permissions::isKnownPermissionKey(permissionKey))
    {
        return false;
    }

    const std::vector<std::string> permissions = loadPermissionsForPosition(dbManager, positionId);
    return containsPermission(permissions, permissionKey);
}

std::unique_ptr<UserAccess> loadUserAccess(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId)
{
    if (!dbManager || !dbManager->getSession() || userId <= 0)
    {
        return nullptr;
    }

    try
    {
        mysqlx::SqlResult result = dbManager->getSession()
                                      ->sql("SELECT u.id, COALESCE(u.account_type, ''), COALESCE(u.position_id, 0), "
                                            "COALESCE(p.name, ''), COALESCE(p.staff_kind, ''), COALESCE(p.system_key, '') "
                                            "FROM users AS u "
                                            "LEFT JOIN positions AS p ON p.id = u.position_id "
                                            "WHERE u.id = ? AND u.is_deleted = 0 "
                                            "LIMIT 1")
                                      .bind(userId)
                                      .execute();

        mysqlx::Row row = result.fetchOne();
        if (!row)
        {
            return nullptr;
        }

        auto access = std::make_unique<UserAccess>();
        access->userId = row[0].get<int>();
        access->accountType = row[1].get<std::string>();
        access->positionId = row[2].get<int>();
        access->positionName = row[3].get<std::string>();
        access->staffKind = row[4].get<std::string>();
        access->systemKey = row[5].get<std::string>();
        access->permissions = loadPermissionsForPosition(dbManager, access->positionId);
        return access;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error loading RBAC access for user " << userId << ": " << e.what() << std::endl;
        return nullptr;
    }
}

bool accessHasPermission(const UserAccess &access, const std::string &permissionKey)
{
    if (!Permissions::isKnownPermissionKey(permissionKey))
    {
        return false;
    }

    if (permissionKey == Permissions::kRbacManage && isSuperAdminSystemPosition(access.systemKey))
    {
        return true;
    }

    return containsPermission(access.permissions, permissionKey);
}

bool userHasPermission(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId,
    const std::string &permissionKey)
{
    if (!Permissions::isKnownPermissionKey(permissionKey))
    {
        return false;
    }

    std::unique_ptr<UserAccess> access = loadUserAccess(dbManager, userId);
    if (!access)
    {
        return false;
    }

    return accessHasPermission(*access, permissionKey);
}

bool accessHasManagementAccess(const UserAccess &access)
{
    return accessHasPermission(access, Permissions::kPortalBoss) ||
           accessHasPermission(access, Permissions::kPortalFinance) ||
           accessHasPermission(access, Permissions::kPortalSuperAdmin);
}

bool userHasManagementAccess(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId)
{
    std::unique_ptr<UserAccess> access = loadUserAccess(dbManager, userId);
    return access && accessHasManagementAccess(*access);
}

EffectiveOrgScope loadEffectiveOrgScope(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId)
{
    // fail-closed：任何异常/无效输入都返回「无组织数据可见」，绝不把错误当"不限制"。
    EffectiveOrgScope scope;
    std::set<int> departmentIds;
    if (!dbManager || !dbManager->getSession() || userId <= 0)
    {
        return scope;
    }

    if (userHasPermission(dbManager, userId, Permissions::kScopeAll))
    {
        scope.unrestricted = true;
        return scope;
    }

    try
    {
        mysqlx::SqlResult implicitResult = dbManager->getSession()
                                               ->sql("SELECT p.department_id "
                                                     "FROM users AS u "
                                                     "JOIN positions AS p ON p.id = u.position_id "
                                                     "WHERE u.id = ? AND u.is_deleted = 0 "
                                                     "AND p.department_id IS NOT NULL")
                                               .bind(userId)
                                               .execute();
        for (mysqlx::Row row = implicitResult.fetchOne(); row; row = implicitResult.fetchOne())
        {
            if (!row[0].isNull())
            {
                departmentIds.insert(row[0].get<int>());
            }
        }

        mysqlx::SqlResult explicitDeptResult = dbManager->getSession()
                                                   ->sql("SELECT department_id FROM user_scopes "
                                                         "WHERE user_id = ? AND department_id IS NOT NULL")
                                                   .bind(userId)
                                                   .execute();
        for (mysqlx::Row row = explicitDeptResult.fetchOne(); row; row = explicitDeptResult.fetchOne())
        {
            if (!row[0].isNull())
            {
                departmentIds.insert(row[0].get<int>());
            }
        }

        mysqlx::SqlResult branchResult = dbManager->getSession()
                                            ->sql("SELECT d.id "
                                                  "FROM user_scopes AS us "
                                                  "JOIN departments AS d ON d.branch_id = us.branch_id "
                                                  "WHERE us.user_id = ? AND us.branch_id IS NOT NULL")
                                            .bind(userId)
                                            .execute();
        for (mysqlx::Row row = branchResult.fetchOne(); row; row = branchResult.fetchOne())
        {
            if (!row[0].isNull())
            {
                departmentIds.insert(row[0].get<int>());
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error loading effective org scope for user " << userId << ": " << e.what() << std::endl;
        return EffectiveOrgScope{};
    }

    scope.departmentIds.assign(departmentIds.begin(), departmentIds.end());
    return scope;
}

std::vector<std::string> effectivePermissions(const UserAccess &access)
{
    std::vector<std::string> permissions = access.permissions;
    if (access.accountType == "customer" &&
        !containsPermission(permissions, Permissions::kPortalUser))
    {
        permissions.emplace_back(Permissions::kPortalUser);
    }
    return permissions;
}

int findPositionIdBySystemKey(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const std::string &systemKey)
{
    if (!dbManager || !dbManager->getSession() || systemKey.empty())
    {
        return 0;
    }

    try
    {
        mysqlx::SqlResult result = dbManager->getSession()
                                      ->sql("SELECT id FROM positions WHERE system_key = ? LIMIT 1")
                                      .bind(systemKey)
                                      .execute();
        mysqlx::Row row = result.fetchOne();
        if (!row || row[0].isNull())
        {
            return 0;
        }
        return row[0].get<int>();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error resolving position by system_key '" << systemKey << "': " << e.what() << std::endl;
        return 0;
    }
}

bool isGrantablePermissionKey(const std::string &permissionKey)
{
    return Permissions::isGrantablePermissionKey(permissionKey);
}
}
