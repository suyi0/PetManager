#pragma once

#include "../../database/DatabaseManagerInterface.h"

#include <memory>
#include <string>
#include <vector>

namespace RbacService
{
struct UserAccess
{
    int userId = 0;
    std::string accountType;
    int positionId = 0;
    std::string positionName;
    std::string staffKind;
    std::string systemKey;
    std::vector<std::string> permissions;
};

std::vector<std::string> loadPermissionsForPosition(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId);

bool positionHasPermission(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId,
    const std::string &permissionKey);

std::unique_ptr<UserAccess> loadUserAccess(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId);

// 对已加载的 access 判权（含超管系统职位对 rbac:manage 的隐式持有）。
// 一次 loadUserAccess + 多次 accessHasPermission，避免每个判定重复查库。
bool accessHasPermission(const UserAccess &access, const std::string &permissionKey);

bool userHasPermission(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId,
    const std::string &permissionKey);

// 管理端会话语义（token 刷新/登出吊销/管理表单）：持任一管理门户权限即视为管理端。
// 等价映射：旧 kManagementRoles = 恰好持有 portal:boss/finance/super-admin 的职位集合。
bool userHasManagementAccess(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId);
bool accessHasManagementAccess(const UserAccess &access);

// 组织数据范围（org_scope，DESIGN §16）。
// unrestricted=true 仅当持 scope:all；否则只允许 departmentIds 内的组织数据。
// 查库失败/无职位/客户账户 → unrestricted=false 且 departmentIds 为空 = 无组织数据可见（fail-closed）。
struct EffectiveOrgScope
{
    bool unrestricted = false;
    std::vector<int> departmentIds;
};

EffectiveOrgScope loadEffectiveOrgScope(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId);

// 对外暴露的有效权限集：客户账户隐式追加 portal:user（客户没有职位/权限行，
// 但用户门户是其账户默认能力）。/auth/me 与两个登录响应共用，防三处漂移。
std::vector<std::string> effectivePermissions(const UserAccess &access);

// 按 system_key 定位内置职位（如 'doctor'/'warehouse-admin'）。
// 业务代码定位职位一律用 system_key，不用显示名——名字超管可改，system_key 不可变。
// 找不到返回 0。
int findPositionIdBySystemKey(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const std::string &systemKey);

bool isGrantablePermissionKey(const std::string &permissionKey);
}
