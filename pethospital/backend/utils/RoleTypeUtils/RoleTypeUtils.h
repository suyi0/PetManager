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
}
