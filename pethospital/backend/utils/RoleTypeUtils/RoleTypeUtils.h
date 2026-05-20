#pragma once

#include "../database/DatabaseManager.h"

#include <memory>
#include <string>

namespace RoleTypeUtils
{
    // 获取角色类型ID
    int getRoleId(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        const std::string &roleName);
    
    // 获取角色类型名
    std::string getRoleName(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        int roleId);

    // 获取用户类型名
    std::string getUserRoleName(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        int userId);

    // 判断用户是否为指定角色类型
    bool userHasRole(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        int userId,
        const std::string &roleName);

    // 判断角色是否属于管理门户角色
    bool isManagementRole(const std::string &roleName);

    // 判断角色是否属于总裁端角色
    bool isBossRole(const std::string &roleName);

    // 判断角色是否属于普通用户角色
    bool isNormalUserRole(const std::string &roleName);

    // 判断用户是否属于管理门户角色
    bool userHasManagementRole(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        int userId);

    // 判断用户是否属于总裁端角色
    bool userHasBossRole(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        int userId);

    // 判断角色是否属于人事门户角色
    bool isPersonnelRole(const std::string &roleName);

    // 判断角色是否属于医疗端角色
    bool isMedicalStaffRole(const std::string &roleName);

    // 判断角色是否属于仓储端角色
    bool isWarehouseStaffRole(const std::string &roleName);

    // 判断用户是否属于人事门户角色
    bool userHasPersonnelRole(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        int userId);
}
