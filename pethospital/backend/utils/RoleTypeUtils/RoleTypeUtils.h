#pragma once

#include "../database/DatabaseManager.h"

#include <memory>
#include <string>

// 职位名的展示查询（display-only）。
// 判权一律走 RbacService（position_permissions 库驱动）；业务身份（是不是医生/员工）
// 一律走 users.account_type / positions.staff_kind。本模块禁止再出现按名字的
// 权限/身份判断——那是动态角色下的双轨漏洞（RBAC-DYNAMIC-ROLES-DESIGN.md §6）。
namespace RoleTypeUtils
{
    // 职位显示名（roleId<=0 时返回历史兼容的"普通用户"，仅用于展示/日志）
    std::string getRoleName(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        int roleId);

    // 用户当前职位显示名（客户账户显示"普通用户"）；带 UserRoleCache 读穿透缓存
    std::string getUserRoleName(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        int userId);
}
