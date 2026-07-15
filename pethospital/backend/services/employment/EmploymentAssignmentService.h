#pragma once

#include "../../database/DatabaseManagerInterface.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

// 统一任职写入口：所有改变 users.account_type/position_id 的路径必须调用本服务。
// 事务内锁定 user / employment / 当前职位 / 目标职位，写 employment 历史、审计、outbox；
// 人事对 approval_required 职位与一切 offboard/regularize 只创建 pending assignment，不直接改 users；
// 任意 action 的 break-glass 均需 explicitBreakGlass + rbac:manage + 非空 reason，写真实审计。
// 管理层审批（approve/reject）走 decide，与 assign 共用生效逻辑，禁止 route/handler 直写 users。
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
    Regularize,
    Offboard,
};

struct AssignRequest
{
    int operatorUserId = 0;
    int targetUserId = 0;
    Action action = Action::Onboard;
    // onboard/transfer 必填；offboard/regularize 忽略
    std::optional<int> targetPositionId;
    // 必填：期望当前 position_id；0 表示当前必须是客户（无职位）
    int expectedCurrentPositionId = -1;
    bool hasExpectedCurrentPosition = false;
    ActorMode mode = ActorMode::Personnel;
    std::string reason;
    std::string effectiveFrom; // YYYY-MM-DD，空则今天
    // 任意 action 直接生效 break-glass 必须 explicitBreakGlass + rbac:manage + 非空 reason。
    // Admin 路由可显式设 true；HR 普通派岗不得因持 rbac:manage 静默直通。
    bool explicitBreakGlass = false;
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

enum class DecisionAction
{
    Approve,
    Reject,
};

struct DecisionRequest
{
    int operatorUserId = 0;
    long long requestId = 0;
    DecisionAction action = DecisionAction::Approve;
    std::string reason;
    // 申请自身 row_version 的 CAS 期望值
    int expectedRowVersion = -1;
    bool hasExpectedRowVersion = false;
};

struct DecisionResult
{
    bool ok = false;
    int httpStatus = 400;
    std::string message;
    std::string errorCode;
    long long requestId = 0;
    long long employmentId = 0;
    long long outboxId = 0;
    std::string assignmentStatus;
    std::string decisionAction;
};

struct ListRequestsQuery
{
    int operatorUserId = 0;
    // 空 = 不按 status 过滤；常见值 pending
    std::string status;
    int page = 1;
    int pageSize = 20;
};

struct ListRequestsResult
{
    bool ok = false;
    int httpStatus = 400;
    std::string message;
    std::string errorCode;
    nlohmann::json items = nlohmann::json::array();
    int total = 0;
    int page = 1;
    int pageSize = 20;
};

AssignResult assign(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const AssignRequest &request);

DecisionResult decide(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const DecisionRequest &request);

ListRequestsResult listRequests(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const ListRequestsQuery &query);
}
