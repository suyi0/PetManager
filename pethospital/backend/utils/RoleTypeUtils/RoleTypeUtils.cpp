#include "roleTypeUtils.h"
#include "../../services/redis/userRoleCache/UserRoleCache.h"

namespace RoleTypeUtils
{
std::string getRoleName(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int roleId)
{
    if (!dbManager || !dbManager->getSession())
    {
        return "";
    }
    if (roleId <= 0)
    {
        return "普通用户";
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                   ->sql("SELECT name FROM positions WHERE id = ? LIMIT 1")
                                   .bind(roleId)
                                   .execute();

    auto row = result.fetchOne();
    if (!row || row[0].isNull())
    {
        return "";
    }

    return row[0].get<std::string>();
}

std::string getUserRoleName(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId)
{
    if (!dbManager || !dbManager->getSession() || userId <= 0)
    {
        return "";
    }

    // 读穿透缓存：命中直接返回，省掉热路径上的 users/positions 查询。
    if (std::optional<std::string> cached = UserRoleCache::readCache(userId))
    {
        return cached.value();
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                   ->sql("SELECT CASE "
                                         "WHEN u.account_type = 'customer' THEN '普通用户' "
                                         "ELSE COALESCE(pos.name, '') END AS role_name "
                                         "FROM users AS u "
                                         "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                         "WHERE u.id = ? "
                                         "LIMIT 1")
                                   .bind(userId)
                                   .execute();

    auto row = result.fetchOne();
    if (!row || row[0].isNull())
    {
        return "";
    }

    const std::string roleName = row[0].get<std::string>();
    // 仅缓存非空角色名（空名不缓存，避免把"查不到"固化）。
    UserRoleCache::writeCache(userId, roleName);
    return roleName;
}
}
