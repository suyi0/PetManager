#pragma once

#include "../../database/DatabaseManagerInterface.h"
#include "../../utils/permissions/Permissions.h"

#include <memory>
#include <string>

// 人事任职权限判权：新 employment:* 精确匹配 + 旧 staff-role:write 受限双读。
// staff-role:write 仅当操作人当前职位属于人事域（staff_kind=personnel 或部门 business_domain=personnel）
// 时作为迁移 fallback；Boss 即使历史持有 staff-role:write 也不能借此获得人事写能力。
namespace PersonnelAccess
{
enum class AssignmentAction
{
    Read,
    Onboard,
    Transfer,
    Offboard,
    Regularize,
};

// 将 HTTP action 字符串映射到动作；未知返回 false。
// regularize 不在 assignment PUT 接口；parse 时拒绝，走独立 regularization 路由。
bool parseAssignmentAction(const std::string &action, AssignmentAction &out);

const char *permissionKeyForAction(AssignmentAction action);

// 操作人职位是否属于人事域（双读 staff-role:write 的前置条件）。
bool operatorHoldsPersonnelDomainPosition(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId);

// 精确 action 权限：先查 employment:*，失败再在人事域职位上 fallback staff-role:write。
bool canPerformAssignmentAction(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId,
    AssignmentAction action);

// 读权限：employment:read 或（人事域 + staff-role:write）。
bool canReadEmployment(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId);
}
