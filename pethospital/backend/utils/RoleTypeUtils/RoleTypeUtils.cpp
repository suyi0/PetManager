#include "RoleTypeUtils.h"

namespace RoleTypeUtils
{
int getRoleId(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const std::string &roleName)
{
    if (!dbManager || !dbManager->getSession())
    {
        return 0;
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                   ->sql("SELECT id FROM types WHERE type = ? LIMIT 1")
                                   .bind(roleName)
                                   .execute();

    auto row = result.fetchOne();
    if (!row || row[0].isNull())
    {
        return 0;
    }

    return row[0].get<int>();
}

std::string getRoleName(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int roleId)
{
    if (!dbManager || !dbManager->getSession() || roleId <= 0)
    {
        return "";
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                   ->sql("SELECT type FROM types WHERE id = ? LIMIT 1")
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

    mysqlx::SqlResult result = dbManager->getSession()
                                   ->sql("SELECT t.type "
                                         "FROM users AS u "
                                         "JOIN types AS t ON u.type_id = t.id "
                                         "WHERE u.id = ? "
                                         "LIMIT 1")
                                   .bind(userId)
                                   .execute();

    auto row = result.fetchOne();
    if (!row || row[0].isNull())
    {
        return "";
    }

    return row[0].get<std::string>();
}

bool userHasRole(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId,
    const std::string &roleName)
{
    return getUserRoleName(dbManager, userId) == roleName;
}
}
