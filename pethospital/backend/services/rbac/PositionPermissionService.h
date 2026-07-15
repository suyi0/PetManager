#pragma once

#include "../../database/DatabaseManagerInterface.h"
#include "../../utils/permissions/Permissions.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

// 职位权限写路径的唯一安全下限服务：
// 直接权限 PUT、权限模板套用、派岗策略修改都必须走这里。
// 同一事务内：锁 positions 行 → 读原始 position_permissions → 算 floor → 替换权限 → 写 policy。
namespace PositionPermissionService
{
struct ReplaceResult
{
    bool ok = false;
    std::string errorMessage;
    std::string errorCode;
    Permissions::AssignmentPolicy effectivePolicy = Permissions::AssignmentPolicy::SuperAdminOnly;
    Permissions::AssignmentPolicy requiredFloor = Permissions::AssignmentPolicy::PersonnelDirect;
    std::vector<std::string> permissions;
};

// 在事务内替换职位权限并处理 assignment_policy：
// - requestedPolicy 有值：必须 ≥ 权限地板
// - requestedPolicy 为空：仅在当前策略低于地板时自动抬升，不自动降级
// 原始权限行（含未知 key）参与 floor 计算；未知 key → SuperAdminOnly。
ReplaceResult replacePermissions(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId,
    const std::vector<std::string> &permissions,
    const std::optional<std::string> &requestedPolicy = std::nullopt);

// 仅修改派岗策略（不改权限集），事务内按原始权限行地板校验。
ReplaceResult updateAssignmentPolicy(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId,
    const std::string &requestedPolicy);

// 只读：失败 fail-closed 为 SuperAdminOnly。
Permissions::AssignmentPolicy loadAssignmentPolicy(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId);
}
