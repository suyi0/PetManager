#include "roleTypeUtils.h"
#include <algorithm>
#include <array>

namespace RoleTypeUtils
{
namespace
{
const std::array<std::string, 6> kManagementRoles = {
    "总裁",
    "副总裁",
    "财务总监",
    "财务经理",
    "部门经理",
    "超级管理员"};

const std::array<std::string, 2> kBossRoles = {
    "总裁",
    "副总裁"};

const std::array<std::string, 1> kPersonnelRoles = {
    "人事经理"};

const std::array<std::string, 1> kNormalUserRoles = {
    "普通用户"};

const std::array<std::string, 2> kMedicalStaffRoles = {
    "医生",
    "护士"};

const std::array<std::string, 1> kWarehouseStaffRoles = {
    "仓库管理员"};
}

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

bool isManagementRole(const std::string &roleName)
{
    return std::find(kManagementRoles.begin(), kManagementRoles.end(), roleName) != kManagementRoles.end();
}

bool isBossRole(const std::string &roleName)
{
    return std::find(kBossRoles.begin(), kBossRoles.end(), roleName) != kBossRoles.end();
}

bool isNormalUserRole(const std::string &roleName)
{
    return std::find(kNormalUserRoles.begin(), kNormalUserRoles.end(), roleName) != kNormalUserRoles.end();
}

bool userHasManagementRole(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId)
{
    return isManagementRole(getUserRoleName(dbManager, userId));
}

bool userHasBossRole(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId)
{
    return isBossRole(getUserRoleName(dbManager, userId));
}

bool isPersonnelRole(const std::string &roleName)
{
    return std::find(kPersonnelRoles.begin(), kPersonnelRoles.end(), roleName) != kPersonnelRoles.end();
}

bool isMedicalStaffRole(const std::string &roleName)
{
    return std::find(kMedicalStaffRoles.begin(), kMedicalStaffRoles.end(), roleName) != kMedicalStaffRoles.end();
}

bool isWarehouseStaffRole(const std::string &roleName)
{
    return std::find(kWarehouseStaffRoles.begin(), kWarehouseStaffRoles.end(), roleName) != kWarehouseStaffRoles.end();
}

bool userHasPersonnelRole(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId)
{
    return isPersonnelRole(getUserRoleName(dbManager, userId));
}
}
