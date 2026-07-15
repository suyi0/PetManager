#pragma once

#include "../../database/DatabaseManagerInterface.h"

#include <memory>
#include <optional>
#include <string>

// 统一任职写入口：所有改变 users.account_type/position_id 的路径必须调用本服务。
// 事务内锁定 user / employment / 当前职位 / 目标职位，写 employment 历史、审计、outbox；
// 人事对 approval_required 职位与一切 offboard 只创建 pending assignment，不直接改 users；
// rbac:manage break-glass 可直接 effective，但仍写完整历史。
namespace EmploymentAssignmentService
{
enum class ActorMode
{
    Personnel,
    Admin,
};

enum class Action
{
    Onboard,
    Transfer,
    Offboard,
};

struct AssignRequest
{
    int operatorUserId = 0;
    int targetUserId = 0;
    Action action = Action::Onboard;
    // onboard/transfer 必填；offboard 忽略
    std::optional<int> targetPositionId;
    // 必填：期望当前 position_id；0 表示当前必须是客户（无职位）
    int expectedCurrentPositionId = -1;
    bool hasExpectedCurrentPosition = false;
    ActorMode mode = ActorMode::Personnel;
    std::string reason;
    std::string effectiveFrom; // YYYY-MM-DD，空则今天
};

struct AssignResult
{
    bool ok = false;
    int httpStatus = 400;
    std::string message;
    std::string errorCode;
    int userId = 0;
    int positionId = 0;
    std::string accountType;
    // pending | effective
    std::string assignmentStatus;
    long long assignmentId = 0;
    long long employmentId = 0;
    long long outboxId = 0;
};

AssignResult assign(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const AssignRequest &request);
}
