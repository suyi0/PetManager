#include "EmploymentAssignmentService.h"

#include "EmploymentOutboxDispatcher.h"
#include "../rbac/RbacService.h"
#include "../../utils/Utils.h"
#include "../../utils/permissions/Permissions.h"

#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

namespace EmploymentAssignmentService
{
namespace
{
template <typename ResultT>
ResultT failResult(int httpStatus, const std::string &message, const std::string &code = "")
{
    ResultT result;
    result.ok = false;
    result.httpStatus = httpStatus;
    result.message = message;
    result.errorCode = code;
    return result;
}

AssignResult fail(int httpStatus, const std::string &message, const std::string &code = "")
{
    return failResult<AssignResult>(httpStatus, message, code);
}

DecisionResult failDecision(int httpStatus, const std::string &message, const std::string &code = "")
{
    return failResult<DecisionResult>(httpStatus, message, code);
}

ListRequestsResult failList(int httpStatus, const std::string &message, const std::string &code = "")
{
    return failResult<ListRequestsResult>(httpStatus, message, code);
}

bool departmentInScope(const RbacService::EffectiveOrgScope &scope, int departmentId)
{
    if (scope.unrestricted)
    {
        return true;
    }
    if (departmentId <= 0 || scope.departmentIds.empty())
    {
        return false;
    }
    return std::find(scope.departmentIds.begin(), scope.departmentIds.end(), departmentId) !=
           scope.departmentIds.end();
}

bool operatorHasRbacManage(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId)
{
    return RbacService::userHasPermission(dbManager, operatorUserId, Permissions::kRbacManage);
}

bool operatorHasApprove(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId)
{
    return RbacService::userHasPermission(
        dbManager, operatorUserId, Permissions::kEmploymentAssignmentApprove);
}

bool targetHoldsRbacManage(
    mysqlx::Session &session,
    int targetUserId,
    const std::string &systemKey)
{
    if (systemKey == "super-admin")
    {
        return true;
    }
    mysqlx::Row posPerm = session.sql(
                                     "SELECT 1 FROM users u "
                                     "JOIN position_permissions pp ON pp.position_id = u.position_id "
                                     "WHERE u.id = ? AND pp.permission_key = ? LIMIT 1")
                              .bind(targetUserId, Permissions::kRbacManage)
                              .execute()
                              .fetchOne();
    if (posPerm)
    {
        return true;
    }
    mysqlx::Row userPerm = session.sql(
                                      "SELECT 1 FROM user_permissions WHERE user_id = ? AND permission_key = ? LIMIT 1")
                               .bind(targetUserId, Permissions::kRbacManage)
                               .execute()
                               .fetchOne();
    return static_cast<bool>(userPerm);
}

std::string todayDateString()
{
    return formatDateOnly(boost::posix_time::second_clock::local_time());
}

std::string actionKey(Action action)
{
    switch (action)
    {
    case Action::Onboard:
        return "onboard";
    case Action::Transfer:
        return "transfer";
    case Action::Regularize:
        return "regularize";
    case Action::Offboard:
        return "offboard";
    }
    return "transfer";
}

std::optional<Action> parseActionKey(const std::string &key)
{
    if (key == "onboard")
    {
        return Action::Onboard;
    }
    if (key == "transfer")
    {
        return Action::Transfer;
    }
    if (key == "regularize")
    {
        return Action::Regularize;
    }
    if (key == "offboard")
    {
        return Action::Offboard;
    }
    return std::nullopt;
}

void ensureDoctorOnlineRow(mysqlx::Session &session, int userId)
{
    const std::string todayDate = todayDateString();
    mysqlx::SqlResult existing = session.sql(
                                            "SELECT doctor_id FROM onlineDoctors WHERE doctor_id = ? LIMIT 1")
                                     .bind(userId)
                                     .execute();
    if (existing.count() == 0)
    {
        // onlineDoctors.check_in_time/check_out_time 为 NOT NULL；入职初始化只建离线占位行。
        session.sql("INSERT INTO onlineDoctors (doctor_id, date, check_in_time, check_out_time, status) "
                    "VALUES (?, ?, '00:00:00', '00:00:00', 'offline')")
            .bind(userId, todayDate)
            .execute();
    }
}

struct LockedPosition
{
    int id = 0;
    int departmentId = 0;
    int branchId = 0;
    std::string systemKey;
    std::string staffKind;
    std::string status;
    Permissions::AssignmentPolicy policy = Permissions::AssignmentPolicy::SuperAdminOnly;
};

struct LockedEmployment
{
    long long id = 0;
    std::string status;
    int rowVersion = 1;
    int probationWaived = 0;
};

// 锁职位行后读取原始 position_permissions（含未知 key），用权威 catalog 算 floor。
std::vector<std::string> loadRawPermissionKeysUnderLock(mysqlx::Session &session, int positionId)
{
    std::vector<std::string> keys;
    mysqlx::SqlResult result = session.sql(
                                          "SELECT permission_key FROM position_permissions WHERE position_id = ?")
                                   .bind(positionId)
                                   .execute();
    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
    {
        if (!row[0].isNull())
        {
            keys.push_back(row[0].get<std::string>());
        }
    }
    return keys;
}

// gate = max(stored assignment_policy, catalog floor from raw keys)。未知 DB key → SuperAdminOnly。
Permissions::AssignmentPolicy effectivePolicyGate(
    mysqlx::Session &session,
    int positionId,
    Permissions::AssignmentPolicy storedPolicy)
{
    const auto rawKeys = loadRawPermissionKeysUnderLock(session, positionId);
    const auto floor = Permissions::requiredAssignmentPolicy(rawKeys);
    return Permissions::maxAssignmentPolicy(storedPolicy, floor);
}

std::optional<LockedPosition> lockPosition(mysqlx::Session &session, int positionId)
{
    if (positionId <= 0)
    {
        return std::nullopt;
    }
    mysqlx::Row row = session.sql(
                                 "SELECT p.id, COALESCE(p.department_id, 0), COALESCE(d.branch_id, 0), "
                                 "COALESCE(p.system_key, ''), COALESCE(p.staff_kind, ''), "
                                 "COALESCE(p.status, 'draft'), COALESCE(p.assignment_policy, 'super_admin_only') "
                                 "FROM positions p "
                                 "LEFT JOIN departments d ON d.id = p.department_id "
                                 "WHERE p.id = ? LIMIT 1 FOR UPDATE")
                          .bind(positionId)
                          .execute()
                          .fetchOne();
    if (!row)
    {
        return std::nullopt;
    }
    LockedPosition pos;
    pos.id = row[0].get<int>();
    pos.departmentId = row[1].isNull() ? 0 : row[1].get<int>();
    pos.branchId = row[2].isNull() ? 0 : row[2].get<int>();
    pos.systemKey = row[3].isNull() ? "" : row[3].get<std::string>();
    pos.staffKind = row[4].isNull() ? "" : row[4].get<std::string>();
    pos.status = row[5].isNull() ? "draft" : row[5].get<std::string>();
    pos.policy = Permissions::parseAssignmentPolicy(
        row[6].isNull() ? "super_admin_only" : row[6].get<std::string>());
    return pos;
}

// CAS 递增 employment.row_version；affected != 1 → 调用方回滚 STALE_VERSION。
bool casBumpEmploymentRowVersion(
    mysqlx::Session &session,
    long long employmentId,
    int expectedRowVersion)
{
    auto result = session.sql(
                             "UPDATE employment SET row_version = row_version + 1 "
                             "WHERE id = ? AND row_version = ?")
                      .bind(employmentId, expectedRowVersion)
                      .execute();
    return result.getAffectedItemsCount() == 1;
}

LockedEmployment lockOrCreateEmployment(mysqlx::Session &session, int userId, int createdBy)
{
    mysqlx::Row existing = session.sql(
                                      "SELECT id, status, row_version, COALESCE(probation_waived, 0) "
                                      "FROM employment WHERE user_id = ? LIMIT 1 FOR UPDATE")
                               .bind(userId)
                               .execute()
                               .fetchOne();
    if (existing)
    {
        LockedEmployment emp;
        emp.id = existing[0].get<int64_t>();
        emp.status = existing[1].isNull() ? "draft" : existing[1].get<std::string>();
        emp.rowVersion = existing[2].isNull() ? 1 : existing[2].get<int>();
        emp.probationWaived = existing[3].isNull() ? 0 : existing[3].get<int>();
        return emp;
    }

    if (createdBy > 0)
    {
        session.sql("INSERT INTO employment (user_id, status, created_by) VALUES (?, 'draft', ?)")
            .bind(userId, createdBy)
            .execute();
    }
    else
    {
        session.sql("INSERT INTO employment (user_id, status, created_by) VALUES (?, 'draft', NULL)")
            .bind(userId)
            .execute();
    }

    mysqlx::Row created = session.sql(
                                     "SELECT id, status, row_version, COALESCE(probation_waived, 0) "
                                     "FROM employment WHERE user_id = ? LIMIT 1 FOR UPDATE")
                              .bind(userId)
                              .execute()
                              .fetchOne();
    LockedEmployment emp;
    emp.id = created[0].get<int64_t>();
    emp.status = created[1].isNull() ? "draft" : created[1].get<std::string>();
    emp.rowVersion = created[2].isNull() ? 1 : created[2].get<int>();
    emp.probationWaived = created[3].isNull() ? 0 : created[3].get<int>();
    return emp;
}

void insertWorkflowAudit(
    mysqlx::Session &session,
    const std::string &resourceType,
    long long resourceId,
    const std::string &actionName,
    int operatorId,
    int branchId,
    int departmentId,
    const nlohmann::json &before,
    const nlohmann::json &after,
    const std::string &reason)
{
    std::string sql =
        "INSERT INTO employment_workflow_audit "
        "(resource_type, resource_id, action, operator_id, branch_id, department_id, "
        "before_snapshot, after_snapshot, reason) VALUES (?, ?, ?, ";
    sql += operatorId > 0 ? "?" : "NULL";
    sql += ", ";
    sql += branchId > 0 ? "?" : "NULL";
    sql += ", ";
    sql += departmentId > 0 ? "?" : "NULL";
    sql += ", ?, ?, ?)";

    auto stmt = session.sql(sql).bind(resourceType, resourceId, actionName);
    if (operatorId > 0)
    {
        stmt.bind(operatorId);
    }
    if (branchId > 0)
    {
        stmt.bind(branchId);
    }
    if (departmentId > 0)
    {
        stmt.bind(departmentId);
    }
    stmt.bind(before.dump(), after.dump(), reason.substr(0, 1000)).execute();
}

// v6: 转正仅允许 employment.status='probation'。
// probation_waived=1 的 onboarding 不伪造 regularize；后续由 regular phase compensation 激活 onboarding→active。
// 批准后 → regularization_pending（= awaiting_compensation）。
bool isRegularizableEmployment(const LockedEmployment &employment)
{
    return employment.status == "probation";
}

// v6: 离职仅 probation / regularization_pending / active。
// onboarding 尚未正式入职：拒绝走 onboarding 任职申请 reject + employment.status=rejected，不当离职。
bool isOffboardableEmployment(const LockedEmployment &employment)
{
    return employment.status == "probation" ||
           employment.status == "regularization_pending" ||
           employment.status == "active";
}

struct ApplyEffectiveContext
{
    int userId = 0;
    int operatorUserId = 0;
    Action action = Action::Onboard;
    int fromPositionId = 0;
    int toPositionId = 0;
    std::string oldStaffKind;
    std::string newStaffKind;
    std::string accountType;
    std::string effectiveFrom;
    long long assignmentId = 0;
    long long employmentId = 0;
    int employmentRowVersion = 1;
    int branchId = 0;
    int departmentId = 0;
    bool breakGlass = false;
    std::string reason;
};

// 在已持锁的事务内应用任职生效（users + employment + outbox + audit 片段）。
// 成功返回 outboxId（可为 0）；失败返回 nullopt 并设置 failMessage/failCode/httpStatus。
struct ApplyOutcome
{
    bool ok = false;
    int httpStatus = 400;
    std::string message;
    std::string errorCode;
    long long outboxId = 0;
    nlohmann::json afterSnap;
    std::string accountType;
    int positionId = 0;
    std::string employmentStatus;
};

ApplyOutcome applyEffectiveUnderLock(
    mysqlx::Session &session,
    const ApplyEffectiveContext &ctx,
    const nlohmann::json &beforeSnap)
{
    ApplyOutcome outcome;
    outcome.afterSnap = beforeSnap;
    outcome.afterSnap["assignment_id"] = ctx.assignmentId;
    outcome.afterSnap["assignment_status"] = "effective";
    outcome.afterSnap["action"] = actionKey(ctx.action);
    outcome.afterSnap["break_glass"] = ctx.breakGlass;
    outcome.afterSnap["to_position_id"] =
        ctx.action == Action::Offboard ? nlohmann::json(nullptr) : nlohmann::json(ctx.toPositionId);

    long long outboxId = 0;

    if (ctx.action == Action::Offboard)
    {
        session.sql("UPDATE users SET account_type = 'customer', position_id = NULL WHERE id = ?")
            .bind(ctx.userId)
            .execute();
        auto empUpd = session.sql(
                                 "UPDATE employment SET status = 'separated', separated_at = NOW(), "
                                 "row_version = row_version + 1 WHERE id = ? AND row_version = ?")
                          .bind(ctx.employmentId, ctx.employmentRowVersion)
                          .execute();
        if (empUpd.getAffectedItemsCount() != 1)
        {
            outcome.httpStatus = 409;
            outcome.message = "员工任职版本已变更，请刷新后重试";
            outcome.errorCode = "STALE_VERSION";
            return outcome;
        }
        outcome.accountType = "customer";
        outcome.positionId = 0;
        outcome.employmentStatus = "separated";
        outcome.afterSnap["account_type"] = "customer";
        outcome.afterSnap["position_id"] = 0;
        outcome.afterSnap["employment_status"] = "separated";
    }
    else if (ctx.action == Action::Regularize)
    {
        // 转正任职决定：进入 regularization_pending（等待正式薪酬 = awaiting_compensation）。
        auto empUpd = session.sql(
                                 "UPDATE employment SET status = 'regularization_pending', "
                                 "regularized_at = COALESCE(regularized_at, NOW()), "
                                 "row_version = row_version + 1 WHERE id = ? AND row_version = ?")
                          .bind(ctx.employmentId, ctx.employmentRowVersion)
                          .execute();
        if (empUpd.getAffectedItemsCount() != 1)
        {
            outcome.httpStatus = 409;
            outcome.message = "员工任职版本已变更，请刷新后重试";
            outcome.errorCode = "STALE_VERSION";
            return outcome;
        }
        outcome.accountType = ctx.accountType.empty() ? "staff" : ctx.accountType;
        outcome.positionId = ctx.fromPositionId;
        outcome.employmentStatus = "regularization_pending";
        outcome.afterSnap["account_type"] = outcome.accountType;
        outcome.afterSnap["position_id"] = ctx.fromPositionId;
        outcome.afterSnap["employment_status"] = "regularization_pending";
    }
    else
    {
        // onboard / transfer
        session.sql("UPDATE users SET account_type = 'staff', position_id = ? WHERE id = ?")
            .bind(ctx.toPositionId, ctx.userId)
            .execute();
        if (ctx.action == Action::Onboard)
        {
            auto empUpd = session.sql(
                                     "UPDATE employment SET status = 'onboarding', hire_date = COALESCE(hire_date, ?), "
                                     "row_version = row_version + 1 WHERE id = ? AND row_version = ?")
                              .bind(ctx.effectiveFrom, ctx.employmentId, ctx.employmentRowVersion)
                              .execute();
            if (empUpd.getAffectedItemsCount() != 1)
            {
                outcome.httpStatus = 409;
                outcome.message = "员工任职版本已变更，请刷新后重试";
                outcome.errorCode = "STALE_VERSION";
                return outcome;
            }
            outcome.employmentStatus = "onboarding";
            outcome.afterSnap["employment_status"] = "onboarding";
        }
        else
        {
            if (!casBumpEmploymentRowVersion(session, ctx.employmentId, ctx.employmentRowVersion))
            {
                outcome.httpStatus = 409;
                outcome.message = "员工任职版本已变更，请刷新后重试";
                outcome.errorCode = "STALE_VERSION";
                return outcome;
            }
            outcome.employmentStatus = beforeSnap.value("employment_status", "");
            outcome.afterSnap["employment_status"] = outcome.employmentStatus;
        }
        outcome.accountType = "staff";
        outcome.positionId = ctx.toPositionId;
        outcome.afterSnap["account_type"] = "staff";
        outcome.afterSnap["position_id"] = ctx.toPositionId;
        if (ctx.newStaffKind == "doctor")
        {
            ensureDoctorOnlineRow(session, ctx.userId);
        }
    }

    // v6: 仅职位变化 / 离职清理个人例外权限；转正职位未变，保留已批准的个人权限。
    if (ctx.action == Action::Onboard ||
        ctx.action == Action::Transfer ||
        ctx.action == Action::Offboard)
    {
        session.sql("DELETE FROM user_permissions WHERE user_id = ?")
            .bind(ctx.userId)
            .execute();
    }

    nlohmann::json payload = {
        {"assignment_id", ctx.assignmentId},
        {"action", actionKey(ctx.action)},
        {"account_type", outcome.accountType},
        {"old_position_id", ctx.fromPositionId},
        {"new_position_id", ctx.action == Action::Offboard ? 0 : (ctx.action == Action::Regularize ? ctx.fromPositionId : ctx.toPositionId)},
        {"old_staff_kind", ctx.oldStaffKind},
        {"new_staff_kind", ctx.action == Action::Offboard ? "" : ctx.newStaffKind},
        {"employment_status", outcome.employmentStatus},
        // effective 路径需要会话撤销 / 设备同步
        {"side_effects_required", true},
    };
    const std::string eventType =
        ctx.action == Action::Offboard ? "employment_separated" : "assignment_changed";
    const std::string eventKey =
        "assignment:" + std::to_string(ctx.assignmentId) + ":" + eventType;
    auto outboxIns = session.sql(
                                "INSERT INTO employment_event_outbox "
                                "(event_key, employment_id, user_id, event_type, payload, status) "
                                "VALUES (?, ?, ?, ?, ?, 'pending')")
                         .bind(eventKey, ctx.employmentId, ctx.userId, eventType, payload.dump())
                         .execute();
    outboxId = static_cast<long long>(outboxIns.getAutoIncrementValue());
    outcome.afterSnap["outbox_id"] = outboxId;
    outcome.outboxId = outboxId;
    outcome.ok = true;
    outcome.httpStatus = 200;
    return outcome;
}

std::string joinDepartmentIds(const std::vector<int> &ids)
{
    std::ostringstream stream;
    for (std::size_t i = 0; i < ids.size(); ++i)
    {
        if (i > 0)
        {
            stream << ",";
        }
        stream << ids[i];
    }
    return stream.str();
}
} // namespace

AssignResult assign(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const AssignRequest &request)
{
    if (!dbManager || !dbManager->getSession())
    {
        return fail(500, "数据库不可用");
    }
    if (request.operatorUserId <= 0 || request.targetUserId <= 0)
    {
        return fail(400, "用户ID无效", "INVALID_USER");
    }
    if (!request.hasExpectedCurrentPosition || request.expectedCurrentPositionId < 0)
    {
        return fail(400, "expected_current_position_id is required", "EXPECTED_POSITION_REQUIRED");
    }
    if (request.operatorUserId == request.targetUserId)
    {
        return fail(403, "不能给自己派岗、调岗或解除任职", "SELF_ASSIGNMENT");
    }
    if (request.reason.empty())
    {
        return fail(400, "原因不能为空", "REASON_REQUIRED");
    }

    const bool isOffboard = request.action == Action::Offboard;
    const bool isRegularize = request.action == Action::Regularize;
    if (!isOffboard && !isRegularize)
    {
        if (!request.targetPositionId.has_value() || request.targetPositionId.value() <= 0)
        {
            return fail(400, "目标职位不能为空", "POSITION_REQUIRED");
        }
    }

    // v6: 任意 action 的 break-glass 均需 explicitBreakGlass + rbac:manage + 非空 reason。
    // 不得仅因持 rbac:manage 在 HR 路由静默直通 onboard/transfer；Admin 路由可显式设 true。
    const bool holdsRbacManage = operatorHasRbacManage(dbManager, request.operatorUserId);
    if (request.explicitBreakGlass && !holdsRbacManage)
    {
        return fail(403, "仅超级管理员可执行 break-glass 直接生效", "BREAK_GLASS_DENIED");
    }
    if (request.explicitBreakGlass && request.reason.empty())
    {
        return fail(400, "break-glass 必须提供非空原因", "REASON_REQUIRED");
    }

    const bool breakGlass =
        request.explicitBreakGlass && holdsRbacManage && !request.reason.empty();

    const RbacService::EffectiveOrgScope scope =
        RbacService::loadEffectiveOrgScope(dbManager, request.operatorUserId);
    const std::string effectiveFrom =
        request.effectiveFrom.empty() ? todayDateString() : request.effectiveFrom;

    long long outboxId = 0;
    AssignResult okResult;
    okResult.ok = false;

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        mysqlx::Row userRow = session->sql(
                                         "SELECT id, COALESCE(account_type, ''), COALESCE(position_id, 0), "
                                         "COALESCE(is_deleted, 0) "
                                         "FROM users WHERE id = ? LIMIT 1 FOR UPDATE")
                                  .bind(request.targetUserId)
                                  .execute()
                                  .fetchOne();
        if (!userRow || (!userRow[3].isNull() && userRow[3].get<int>() != 0))
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "用户不存在", "NOT_FOUND");
        }

        const int userId = userRow[0].get<int>();
        const std::string accountType = userRow[1].isNull() ? "" : userRow[1].get<std::string>();
        const int positionId = userRow[2].isNull() ? 0 : userRow[2].get<int>();

        if (positionId != request.expectedCurrentPositionId)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "员工当前位置已变更，请刷新后重试", "STALE_VERSION");
        }

        std::optional<LockedPosition> currentPosition;
        if (positionId > 0)
        {
            currentPosition = lockPosition(*session, positionId);
            if (!currentPosition.has_value())
            {
                rollbackTransactionQuietly(*session);
                return fail(500, "当前职位数据异常", "CURRENT_POSITION_MISSING");
            }
        }

        const std::string currentSystemKey =
            currentPosition.has_value() ? currentPosition->systemKey : "";
        if (request.mode == ActorMode::Personnel &&
            targetHoldsRbacManage(*session, userId, currentSystemKey))
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "用户不存在", "NOT_FOUND");
        }
        if (currentSystemKey == "super-admin")
        {
            rollbackTransactionQuietly(*session);
            return fail(403, "不能通过接口改派系统超级管理员", "SUPER_ADMIN_LOCKED");
        }

        std::optional<LockedPosition> targetPosition;
        if (!isOffboard && !isRegularize)
        {
            targetPosition = lockPosition(*session, request.targetPositionId.value());
            if (!targetPosition.has_value())
            {
                rollbackTransactionQuietly(*session);
                return fail(404, "岗位不存在", "POSITION_NOT_FOUND");
            }
            if (targetPosition->systemKey == "super-admin")
            {
                rollbackTransactionQuietly(*session);
                return fail(403, "不能通过接口授予系统超级管理员岗位", "SUPER_ADMIN_LOCKED");
            }
            if (targetPosition->status != "published")
            {
                rollbackTransactionQuietly(*session);
                return fail(400, "只能派到已发布职位", "POSITION_NOT_PUBLISHED");
            }
        }

        if (request.action == Action::Onboard && positionId > 0)
        {
            rollbackTransactionQuietly(*session);
            return fail(400, "员工已有任职，请使用调岗", "ALREADY_STAFF");
        }
        if (request.action == Action::Transfer && positionId <= 0)
        {
            rollbackTransactionQuietly(*session);
            return fail(400, "员工尚未任职，请使用入职", "NOT_STAFF");
        }
        if ((isOffboard || isRegularize) && positionId <= 0)
        {
            rollbackTransactionQuietly(*session);
            return fail(400, "员工当前无任职", "NOT_STAFF");
        }
        if (request.action == Action::Transfer &&
            targetPosition.has_value() &&
            targetPosition->id == positionId)
        {
            rollbackTransactionQuietly(*session);
            return fail(400, "目标职位与当前职位相同", "SAME_POSITION");
        }

        if (currentPosition.has_value() &&
            currentPosition->departmentId > 0 &&
            !departmentInScope(scope, currentPosition->departmentId))
        {
            rollbackTransactionQuietly(*session);
            return fail(403, "目标员工不在组织范围内", "OUT_OF_SCOPE");
        }
        if (targetPosition.has_value() &&
            !departmentInScope(scope, targetPosition->departmentId))
        {
            rollbackTransactionQuietly(*session);
            return fail(403, "目标职位不在组织范围内", "OUT_OF_SCOPE");
        }

        LockedEmployment employment = lockOrCreateEmployment(*session, userId, request.operatorUserId);

        if (isRegularize && !isRegularizableEmployment(employment))
        {
            rollbackTransactionQuietly(*session);
            // 不用 422：Crow 1.2.x 状态码表无 422，未知码回落 500。
            return fail(400, "当前任职状态不可申请转正", "INVALID_EMPLOYMENT_STATE");
        }
        if (isOffboard && !isOffboardableEmployment(employment))
        {
            rollbackTransactionQuietly(*session);
            return fail(400, "当前任职状态不可申请离职", "INVALID_EMPLOYMENT_STATE");
        }

        mysqlx::Row openAssignment = session->sql(
                                                "SELECT id FROM employment_assignment "
                                                "WHERE employment_id = ? AND status IN ('pending','approved') "
                                                "LIMIT 1 FOR UPDATE")
                                         .bind(employment.id)
                                         .execute()
                                         .fetchOne();
        if (openAssignment)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "已有进行中的任职申请", "OPEN_ASSIGNMENT_EXISTS");
        }

        // B6: 不能只信 positions.assignment_policy。
        // 锁职位后读原始 position_permissions，catalog 算 floor，gate = max(stored, floor)。
        // offboard / regularize 始终至少 approval_required（再与 floor 取 max）。
        Permissions::AssignmentPolicy gate = Permissions::AssignmentPolicy::PersonnelDirect;
        if (isOffboard || isRegularize)
        {
            gate = Permissions::AssignmentPolicy::ApprovalRequired;
            if (currentPosition.has_value())
            {
                gate = Permissions::maxAssignmentPolicy(
                    gate,
                    effectivePolicyGate(*session, currentPosition->id, currentPosition->policy));
            }
        }
        else if (targetPosition.has_value())
        {
            gate = effectivePolicyGate(*session, targetPosition->id, targetPosition->policy);
        }

        if (!breakGlass && gate == Permissions::AssignmentPolicy::SuperAdminOnly)
        {
            rollbackTransactionQuietly(*session);
            return fail(403, "该职位仅超级管理员可派岗", "POLICY_DENIED");
        }

        // 非 break-glass：approval_required / offboard / regularize / super_admin_only → pending；
        // 仅 gate==personnel_direct 的 onboard/transfer 可直接 effective。
        const bool makeEffective =
            breakGlass ||
            (!isOffboard && !isRegularize && gate == Permissions::AssignmentPolicy::PersonnelDirect);

        int branchId = 0;
        int departmentId = 0;
        if (targetPosition.has_value())
        {
            branchId = targetPosition->branchId;
            departmentId = targetPosition->departmentId;
        }
        else if (currentPosition.has_value())
        {
            branchId = currentPosition->branchId;
            departmentId = currentPosition->departmentId;
        }
        if (branchId <= 0)
        {
            mysqlx::Row mainBranch = session->sql(
                                                "SELECT id FROM branches WHERE system_key = 'main' LIMIT 1")
                                         .execute()
                                         .fetchOne();
            branchId = mainBranch ? mainBranch[0].get<int>() : 0;
        }
        if (branchId <= 0 || departmentId <= 0)
        {
            rollbackTransactionQuietly(*session);
            return fail(400, "无法解析组织范围快照", "SCOPE_SNAPSHOT_MISSING");
        }

        const std::string oldStaffKind =
            currentPosition.has_value() ? currentPosition->staffKind : "";
        const std::string newStaffKind =
            isOffboard ? ""
                       : (isRegularize
                              ? oldStaffKind
                              : (targetPosition.has_value() ? targetPosition->staffKind : ""));
        const std::string assignmentStatus = makeEffective ? "effective" : "pending";
        const int toPositionId =
            isOffboard ? 0
                       : (isRegularize ? positionId : request.targetPositionId.value());

        nlohmann::json beforeSnap = {
            {"user_id", userId},
            {"account_type", accountType},
            {"position_id", positionId},
            {"employment_status", employment.status},
            {"employment_row_version", employment.rowVersion},
            {"probation_waived", employment.probationWaived},
            {"org_branch_id", branchId},
            {"org_department_id", departmentId},
        };

        // 插入 assignment 历史。
        // 直接 effective（personnel_direct 或 break-glass）不填 reviewed_by：
        // 职责分离约束要求 reviewed_by 为空或不同于 requested_by；直接生效没有独立审批人。
        long long assignmentId = 0;
        const std::string statusSql = makeEffective ? "effective" : "pending";
        if (isOffboard)
        {
            auto ins = session->sql(
                                  "INSERT INTO employment_assignment "
                                  "(employment_id, branch_id, department_id, from_position_id, to_position_id, "
                                  "action, status, effective_from, reason, request_source, requested_by, "
                                  "expected_employment_row_version) "
                                  "VALUES (?, ?, ?, ?, NULL, ?, ?, ?, ?, 'user', ?, ?)")
                           .bind(employment.id, branchId, departmentId, positionId,
                                 actionKey(request.action), statusSql, effectiveFrom,
                                 request.reason.substr(0, 500), request.operatorUserId,
                                 employment.rowVersion)
                           .execute();
            assignmentId = static_cast<long long>(ins.getAutoIncrementValue());
        }
        else if (isRegularize)
        {
            auto ins = session->sql(
                                  "INSERT INTO employment_assignment "
                                  "(employment_id, branch_id, department_id, from_position_id, to_position_id, "
                                  "action, status, effective_from, reason, request_source, requested_by, "
                                  "expected_employment_row_version) "
                                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 'user', ?, ?)")
                           .bind(employment.id, branchId, departmentId, positionId, positionId,
                                 actionKey(request.action), statusSql, effectiveFrom,
                                 request.reason.substr(0, 500), request.operatorUserId,
                                 employment.rowVersion)
                           .execute();
            assignmentId = static_cast<long long>(ins.getAutoIncrementValue());
        }
        else if (positionId > 0)
        {
            auto ins = session->sql(
                                  "INSERT INTO employment_assignment "
                                  "(employment_id, branch_id, department_id, from_position_id, to_position_id, "
                                  "action, status, effective_from, reason, request_source, requested_by, "
                                  "expected_employment_row_version) "
                                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 'user', ?, ?)")
                           .bind(employment.id, branchId, departmentId, positionId,
                                 request.targetPositionId.value(), actionKey(request.action),
                                 statusSql, effectiveFrom, request.reason.substr(0, 500),
                                 request.operatorUserId, employment.rowVersion)
                           .execute();
            assignmentId = static_cast<long long>(ins.getAutoIncrementValue());
        }
        else
        {
            auto ins = session->sql(
                                  "INSERT INTO employment_assignment "
                                  "(employment_id, branch_id, department_id, from_position_id, to_position_id, "
                                  "action, status, effective_from, reason, request_source, requested_by, "
                                  "expected_employment_row_version) "
                                  "VALUES (?, ?, ?, NULL, ?, ?, ?, ?, ?, 'user', ?, ?)")
                           .bind(employment.id, branchId, departmentId,
                                 request.targetPositionId.value(), actionKey(request.action),
                                 statusSql, effectiveFrom, request.reason.substr(0, 500),
                                 request.operatorUserId, employment.rowVersion)
                           .execute();
            assignmentId = static_cast<long long>(ins.getAutoIncrementValue());
        }

        nlohmann::json afterSnap = beforeSnap;
        afterSnap["assignment_id"] = assignmentId;
        afterSnap["assignment_status"] = assignmentStatus;
        afterSnap["action"] = actionKey(request.action);
        afterSnap["to_position_id"] =
            isOffboard ? nlohmann::json(nullptr) : nlohmann::json(toPositionId);
        afterSnap["break_glass"] = breakGlass;

        if (makeEffective)
        {
            ApplyEffectiveContext applyCtx;
            applyCtx.userId = userId;
            applyCtx.operatorUserId = request.operatorUserId;
            applyCtx.action = request.action;
            applyCtx.fromPositionId = positionId;
            applyCtx.toPositionId = toPositionId;
            applyCtx.oldStaffKind = oldStaffKind;
            applyCtx.newStaffKind = newStaffKind;
            applyCtx.accountType = accountType;
            applyCtx.effectiveFrom = effectiveFrom;
            applyCtx.assignmentId = assignmentId;
            applyCtx.employmentId = employment.id;
            applyCtx.employmentRowVersion = employment.rowVersion;
            applyCtx.branchId = branchId;
            applyCtx.departmentId = departmentId;
            // 真实 break-glass：普通 personnel_direct 为 false，仅显式 break-glass 为 true
            applyCtx.breakGlass = breakGlass;
            applyCtx.reason = request.reason;

            const ApplyOutcome applied = applyEffectiveUnderLock(*session, applyCtx, beforeSnap);
            if (!applied.ok)
            {
                rollbackTransactionQuietly(*session);
                return fail(applied.httpStatus, applied.message, applied.errorCode);
            }
            afterSnap = applied.afterSnap;
            afterSnap["break_glass"] = breakGlass;
            outboxId = applied.outboxId;
        }
        else
        {
            // create pending 也写 outbox（无设备/会话副作用）
            nlohmann::json payload = {
                {"assignment_id", assignmentId},
                {"action", actionKey(request.action)},
                {"assignment_status", "pending"},
                {"account_type", accountType},
                {"old_position_id", positionId},
                {"new_position_id", isOffboard ? 0 : toPositionId},
                {"employment_status", employment.status},
                {"employment_row_version", employment.rowVersion},
                {"org_branch_id", branchId},
                {"org_department_id", departmentId},
                {"side_effects_required", false},
            };
            const std::string eventKey =
                "assignment:" + std::to_string(assignmentId) + ":requested";
            auto outboxIns = session->sql(
                                        "INSERT INTO employment_event_outbox "
                                        "(event_key, employment_id, user_id, event_type, payload, status) "
                                        "VALUES (?, ?, ?, 'assignment_changed', ?, 'pending')")
                                 .bind(eventKey, employment.id, userId, payload.dump())
                                 .execute();
            outboxId = static_cast<long long>(outboxIns.getAutoIncrementValue());
            afterSnap["outbox_id"] = outboxId;
        }

        // 任意 break-glass 写 assignment_break_glass_effective；普通 personnel_direct 写 assignment_effective
        const std::string auditAction =
            makeEffective
                ? (breakGlass ? "assignment_break_glass_effective" : "assignment_effective")
                : "assignment_pending";

        insertWorkflowAudit(
            *session,
            "assignment",
            assignmentId,
            auditAction,
            request.operatorUserId,
            branchId,
            departmentId,
            beforeSnap,
            afterSnap,
            request.reason);

        insertWorkflowAudit(
            *session,
            "employment",
            employment.id,
            makeEffective ? "employment_snapshot_updated" : "employment_assignment_requested",
            request.operatorUserId,
            branchId,
            departmentId,
            beforeSnap,
            afterSnap,
            request.reason);

        session->sql("COMMIT").execute();

        okResult.ok = true;
        okResult.httpStatus = 200;
        okResult.message = makeEffective ? "任职变更已生效" : "任职申请已提交，等待审批";
        okResult.userId = userId;
        okResult.positionId = makeEffective
                                  ? (isOffboard ? 0 : (isRegularize ? positionId : toPositionId))
                                  : positionId;
        okResult.accountType = makeEffective
                                   ? (isOffboard ? "customer" : "staff")
                                   : accountType;
        okResult.assignmentStatus = assignmentStatus;
        okResult.assignmentId = assignmentId;
        okResult.employmentId = employment.id;
        okResult.outboxId = outboxId;
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        std::cerr << "EmploymentAssignmentService::assign failed: " << e.what() << std::endl;
        const std::string what = e.what();
        if (what.find("Duplicate") != std::string::npos ||
            what.find("uq_employment_assignment_open") != std::string::npos)
        {
            return fail(409, "已有进行中的任职申请", "OPEN_ASSIGNMENT_EXISTS");
        }
        return fail(500, std::string("任职变更失败: ") + e.what());
    }

    if (okResult.ok && outboxId > 0)
    {
        EmploymentOutboxDispatcher::dispatchOne(dbManager, outboxId);
    }
    return okResult;
}

DecisionResult decide(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const DecisionRequest &request)
{
    if (!dbManager || !dbManager->getSession())
    {
        return failDecision(500, "数据库不可用");
    }
    if (request.operatorUserId <= 0 || request.requestId <= 0)
    {
        return failDecision(400, "参数无效", "INVALID_REQUEST");
    }
    if (!request.hasExpectedRowVersion || request.expectedRowVersion < 1)
    {
        return failDecision(400, "expectedRowVersion 必填", "EXPECTED_ROW_VERSION_REQUIRED");
    }
    if (request.reason.empty())
    {
        return failDecision(400, "原因不能为空", "REASON_REQUIRED");
    }
    // 精确权限：employment-assignment:approve，禁止用 management 门槛替代。
    if (!operatorHasApprove(dbManager, request.operatorUserId))
    {
        return failDecision(403, "缺少任职审批权限", "APPROVE_DENIED");
    }

    const RbacService::EffectiveOrgScope scope =
        RbacService::loadEffectiveOrgScope(dbManager, request.operatorUserId);
    const bool isApprove = request.action == DecisionAction::Approve;
    const std::string decisionKey = isApprove ? "approve" : "reject";

    long long outboxId = 0;
    DecisionResult okResult;
    okResult.ok = false;

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        // 双锁：先锁 request，再锁 employment（固定顺序避免死锁）。
        mysqlx::Row reqRow = session->sql(
                                        "SELECT ea.id, ea.employment_id, ea.branch_id, ea.department_id, "
                                        "ea.from_position_id, ea.to_position_id, ea.action, ea.status, "
                                        "ea.requested_by, ea.expected_employment_row_version, ea.row_version, "
                                        "ea.effective_from, ea.reason "
                                        "FROM employment_assignment ea "
                                        "WHERE ea.id = ? LIMIT 1 FOR UPDATE")
                                 .bind(request.requestId)
                                 .execute()
                                 .fetchOne();
        if (!reqRow)
        {
            rollbackTransactionQuietly(*session);
            // 猜测越权 ID → 404
            return failDecision(404, "申请不存在", "NOT_FOUND");
        }

        const long long assignmentId = reqRow[0].get<int64_t>();
        const long long employmentId = reqRow[1].get<int64_t>();
        const int branchId = reqRow[2].isNull() ? 0 : reqRow[2].get<int>();
        const int departmentId = reqRow[3].isNull() ? 0 : reqRow[3].get<int>();
        const int fromPositionId = reqRow[4].isNull() ? 0 : reqRow[4].get<int>();
        const int toPositionId = reqRow[5].isNull() ? 0 : reqRow[5].get<int>();
        const std::string actionStr = reqRow[6].isNull() ? "" : reqRow[6].get<std::string>();
        const std::string status = reqRow[7].isNull() ? "" : reqRow[7].get<std::string>();
        const int requestedBy = reqRow[8].isNull() ? 0 : reqRow[8].get<int>();
        const int expectedEmpRowVersion = reqRow[9].isNull() ? 1 : reqRow[9].get<int>();
        const int requestRowVersion = reqRow[10].isNull() ? 1 : reqRow[10].get<int>();
        const std::string effectiveFrom =
            reqRow[11].isNull() ? todayDateString() : reqRow[11].get<std::string>();

        // 组织范围按申请快照过滤；超管 unrestricted；越权猜 ID → 404
        if (!departmentInScope(scope, departmentId))
        {
            rollbackTransactionQuietly(*session);
            return failDecision(404, "申请不存在", "NOT_FOUND");
        }

        if (status != "pending")
        {
            rollbackTransactionQuietly(*session);
            return failDecision(409, "申请已处理，不能重复决策", "ALREADY_DECIDED");
        }
        if (requestRowVersion != request.expectedRowVersion)
        {
            rollbackTransactionQuietly(*session);
            return failDecision(409, "申请版本已变更，请刷新后重试", "STALE_VERSION");
        }
        // 申请人不得审批自己的申请
        if (requestedBy > 0 && requestedBy == request.operatorUserId)
        {
            rollbackTransactionQuietly(*session);
            return failDecision(403, "不能审批自己发起的任职申请", "SELF_APPROVAL");
        }

        const auto actionOpt = parseActionKey(actionStr);
        if (!actionOpt.has_value())
        {
            rollbackTransactionQuietly(*session);
            // 不用 422：Crow 1.2.x 状态码表无 422，未知码回落 500。
            return failDecision(400, "未知任职动作", "UNKNOWN_ACTION");
        }
        const Action action = actionOpt.value();

        mysqlx::Row empRow = session->sql(
                                        "SELECT e.id, e.user_id, e.status, e.row_version, COALESCE(e.probation_waived, 0), "
                                        "COALESCE(u.account_type, ''), COALESCE(u.position_id, 0), COALESCE(u.is_deleted, 0) "
                                        "FROM employment e "
                                        "JOIN users u ON u.id = e.user_id "
                                        "WHERE e.id = ? LIMIT 1 FOR UPDATE")
                                 .bind(employmentId)
                                 .execute()
                                 .fetchOne();
        if (!empRow)
        {
            rollbackTransactionQuietly(*session);
            return failDecision(404, "申请不存在", "NOT_FOUND");
        }

        const int targetUserId = empRow[1].get<int>();
        const std::string employmentStatus = empRow[2].isNull() ? "draft" : empRow[2].get<std::string>();
        const int employmentRowVersion = empRow[3].isNull() ? 1 : empRow[3].get<int>();
        const int probationWaived = empRow[4].isNull() ? 0 : empRow[4].get<int>();
        const std::string accountType = empRow[5].isNull() ? "" : empRow[5].get<std::string>();
        const int currentPositionId = empRow[6].isNull() ? 0 : empRow[6].get<int>();
        const int isDeleted = empRow[7].isNull() ? 0 : empRow[7].get<int>();

        if (isDeleted != 0)
        {
            rollbackTransactionQuietly(*session);
            return failDecision(404, "申请不存在", "NOT_FOUND");
        }

        // 锁当前职位以检查 super-admin / rbac:manage
        std::string currentSystemKey;
        std::string oldStaffKind;
        if (currentPositionId > 0)
        {
            auto curPos = lockPosition(*session, currentPositionId);
            if (curPos.has_value())
            {
                currentSystemKey = curPos->systemKey;
                oldStaffKind = curPos->staffKind;
            }
        }
        if (targetHoldsRbacManage(*session, targetUserId, currentSystemKey))
        {
            rollbackTransactionQuietly(*session);
            return failDecision(404, "申请不存在", "NOT_FOUND");
        }

        nlohmann::json beforeSnap = {
            {"request_id", assignmentId},
            {"user_id", targetUserId},
            {"account_type", accountType},
            {"position_id", currentPositionId},
            {"employment_status", employmentStatus},
            {"employment_row_version", employmentRowVersion},
            {"assignment_row_version", requestRowVersion},
            {"assignment_status", status},
            {"action", actionStr},
            {"requested_by", requestedBy},
            {"org_branch_id", branchId},
            {"org_department_id", departmentId},
            {"expected_employment_row_version", expectedEmpRowVersion},
        };

        if (!isApprove)
        {
            // reject：结束 request。onboarding reject 额外 CAS 将 employment → rejected；
            // 其他 action 的 reject 不改 employment。
            auto upd = session->sql(
                                  "UPDATE employment_assignment "
                                  "SET status = 'rejected', reviewed_by = ?, reviewed_at = NOW(), "
                                  "row_version = row_version + 1 "
                                  "WHERE id = ? AND status = 'pending' AND row_version = ?")
                           .bind(request.operatorUserId, assignmentId, request.expectedRowVersion)
                           .execute();
            if (upd.getAffectedItemsCount() != 1)
            {
                rollbackTransactionQuietly(*session);
                return failDecision(409, "申请版本已变更，请刷新后重试", "STALE_VERSION");
            }

            nlohmann::json afterSnap = beforeSnap;
            afterSnap["assignment_status"] = "rejected";
            afterSnap["reviewed_by"] = request.operatorUserId;
            afterSnap["decision"] = "reject";

            if (action == Action::Onboard)
            {
                // 入职拒绝：锁已持有；校验 expected_employment_row_version + 仍处入职前状态。
                if (employmentRowVersion != expectedEmpRowVersion)
                {
                    rollbackTransactionQuietly(*session);
                    return failDecision(409, "员工任职版本已变更，请刷新后重试", "STALE_VERSION");
                }
                if (currentPositionId != 0 || fromPositionId != 0)
                {
                    rollbackTransactionQuietly(*session);
                    return failDecision(409, "员工任职状态已变更，无法拒绝入职申请", "STATE_CONFLICT");
                }
                // 入职前 draft（或同态 re-hire 前的 rejected 再次申请失败后仍可再拒）；
                // CAS 要求仍为申请时版本，并写入 rejected。
                if (employmentStatus != "draft" && employmentStatus != "rejected")
                {
                    rollbackTransactionQuietly(*session);
                    return failDecision(409, "员工任职状态已变更，无法拒绝入职申请", "STATE_CONFLICT");
                }
                auto empUpd = session->sql(
                                         "UPDATE employment SET status = 'rejected', "
                                         "row_version = row_version + 1 "
                                         "WHERE id = ? AND row_version = ? "
                                         "AND status IN ('draft', 'rejected')")
                                  .bind(employmentId, expectedEmpRowVersion)
                                  .execute();
                if (empUpd.getAffectedItemsCount() != 1)
                {
                    rollbackTransactionQuietly(*session);
                    return failDecision(409, "员工任职版本已变更，请刷新后重试", "STALE_VERSION");
                }
                afterSnap["employment_status"] = "rejected";
                afterSnap["employment_row_version"] = expectedEmpRowVersion + 1;

                insertWorkflowAudit(
                    *session,
                    "employment",
                    employmentId,
                    "employment_rejected",
                    request.operatorUserId,
                    branchId,
                    departmentId,
                    beforeSnap,
                    afterSnap,
                    request.reason);
            }

            insertWorkflowAudit(
                *session,
                "assignment",
                assignmentId,
                "assignment_rejected",
                request.operatorUserId,
                branchId,
                departmentId,
                beforeSnap,
                afterSnap,
                request.reason);

            // reject 写 outbox，显式关闭副作用（不得靠 status/decision 字符串推断）
            nlohmann::json payload = {
                {"assignment_id", assignmentId},
                {"action", actionStr},
                {"decision", "reject"},
                {"account_type", accountType},
                {"old_position_id", currentPositionId},
                {"new_position_id", currentPositionId},
                {"side_effects_required", false},
            };
            const std::string eventKey =
                "assignment:" + std::to_string(assignmentId) + ":rejected";
            // 复用 assignment_changed 事件类型（enum 仅有两类）
            auto outboxIns = session->sql(
                                        "INSERT INTO employment_event_outbox "
                                        "(event_key, employment_id, user_id, event_type, payload, status) "
                                        "VALUES (?, ?, ?, 'assignment_changed', ?, 'pending')")
                                 .bind(eventKey, employmentId, targetUserId, payload.dump())
                                 .execute();
            outboxId = static_cast<long long>(outboxIns.getAutoIncrementValue());
            afterSnap["outbox_id"] = outboxId;

            session->sql("COMMIT").execute();

            okResult.ok = true;
            okResult.httpStatus = 200;
            okResult.message = "已拒绝任职申请";
            okResult.requestId = assignmentId;
            okResult.employmentId = employmentId;
            okResult.outboxId = outboxId;
            okResult.assignmentStatus = "rejected";
            okResult.decisionAction = "reject";
        }
        else
        {
            // approve：校验 employment CAS + 目标仍处申请时状态
            if (employmentRowVersion != expectedEmpRowVersion)
            {
                rollbackTransactionQuietly(*session);
                return failDecision(409, "员工任职版本已变更，请刷新后重试", "STALE_VERSION");
            }

            // 校验 from/to 职位与当前快照一致性
            if (action == Action::Onboard)
            {
                if (currentPositionId != 0 || fromPositionId != 0)
                {
                    rollbackTransactionQuietly(*session);
                    return failDecision(409, "员工任职状态已变更，无法批准入职", "STATE_CONFLICT");
                }
            }
            else if (action == Action::Transfer || action == Action::Offboard ||
                     action == Action::Regularize)
            {
                if (currentPositionId != fromPositionId || fromPositionId <= 0)
                {
                    rollbackTransactionQuietly(*session);
                    return failDecision(409, "员工职位已变更，无法批准申请", "STATE_CONFLICT");
                }
            }

            LockedEmployment empCheck;
            empCheck.id = employmentId;
            empCheck.status = employmentStatus;
            empCheck.rowVersion = employmentRowVersion;
            empCheck.probationWaived = probationWaived;
            if (action == Action::Regularize && !isRegularizableEmployment(empCheck))
            {
                rollbackTransactionQuietly(*session);
                return failDecision(409, "员工已不在可转正状态", "STATE_CONFLICT");
            }
            if (action == Action::Offboard && !isOffboardableEmployment(empCheck))
            {
                rollbackTransactionQuietly(*session);
                return failDecision(409, "员工已不在可离职状态", "STATE_CONFLICT");
            }

            std::string newStaffKind = oldStaffKind;
            if (action == Action::Onboard || action == Action::Transfer)
            {
                auto tgt = lockPosition(*session, toPositionId);
                if (!tgt.has_value() || tgt->status != "published")
                {
                    rollbackTransactionQuietly(*session);
                    return failDecision(409, "目标职位不可用", "POSITION_UNAVAILABLE");
                }
                if (tgt->systemKey == "super-admin")
                {
                    rollbackTransactionQuietly(*session);
                    return failDecision(403, "不能通过接口授予系统超级管理员岗位", "SUPER_ADMIN_LOCKED");
                }
                newStaffKind = tgt->staffKind;
            }

            // CAS 更新 request → effective
            auto upd = session->sql(
                                  "UPDATE employment_assignment "
                                  "SET status = 'effective', reviewed_by = ?, reviewed_at = NOW(), "
                                  "row_version = row_version + 1 "
                                  "WHERE id = ? AND status = 'pending' AND row_version = ?")
                           .bind(request.operatorUserId, assignmentId, request.expectedRowVersion)
                           .execute();
            if (upd.getAffectedItemsCount() != 1)
            {
                rollbackTransactionQuietly(*session);
                return failDecision(409, "申请版本已变更，请刷新后重试", "STALE_VERSION");
            }

            ApplyEffectiveContext applyCtx;
            applyCtx.userId = targetUserId;
            applyCtx.operatorUserId = request.operatorUserId;
            applyCtx.action = action;
            applyCtx.fromPositionId = fromPositionId;
            applyCtx.toPositionId = toPositionId;
            applyCtx.oldStaffKind = oldStaffKind;
            applyCtx.newStaffKind = newStaffKind;
            applyCtx.accountType = accountType;
            applyCtx.effectiveFrom = effectiveFrom;
            applyCtx.assignmentId = assignmentId;
            applyCtx.employmentId = employmentId;
            applyCtx.employmentRowVersion = employmentRowVersion;
            applyCtx.branchId = branchId;
            applyCtx.departmentId = departmentId;
            applyCtx.breakGlass = false;
            applyCtx.reason = request.reason;

            const ApplyOutcome applied = applyEffectiveUnderLock(*session, applyCtx, beforeSnap);
            if (!applied.ok)
            {
                rollbackTransactionQuietly(*session);
                return failDecision(applied.httpStatus, applied.message, applied.errorCode);
            }
            outboxId = applied.outboxId;

            nlohmann::json afterSnap = applied.afterSnap;
            afterSnap["reviewed_by"] = request.operatorUserId;
            afterSnap["decision"] = "approve";
            afterSnap["assignment_status"] = "effective";
            afterSnap["assignment_row_version"] = request.expectedRowVersion + 1;

            insertWorkflowAudit(
                *session,
                "assignment",
                assignmentId,
                "assignment_approved",
                request.operatorUserId,
                branchId,
                departmentId,
                beforeSnap,
                afterSnap,
                request.reason);

            insertWorkflowAudit(
                *session,
                "employment",
                employmentId,
                "employment_snapshot_updated",
                request.operatorUserId,
                branchId,
                departmentId,
                beforeSnap,
                afterSnap,
                request.reason);

            session->sql("COMMIT").execute();

            okResult.ok = true;
            okResult.httpStatus = 200;
            okResult.message = "任职申请已批准并生效";
            okResult.requestId = assignmentId;
            okResult.employmentId = employmentId;
            okResult.outboxId = outboxId;
            okResult.assignmentStatus = "effective";
            okResult.decisionAction = "approve";
        }
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        std::cerr << "EmploymentAssignmentService::decide failed: " << e.what() << std::endl;
        const std::string what = e.what();
        // 职责分离 DB 约束
        if (what.find("chk_ea_separation_of_duties") != std::string::npos)
        {
            return failDecision(403, "不能审批自己发起的任职申请", "SELF_APPROVAL");
        }
        return failDecision(500, std::string("审批失败: ") + e.what());
    }

    if (okResult.ok && outboxId > 0)
    {
        EmploymentOutboxDispatcher::dispatchOne(dbManager, outboxId);
    }
    return okResult;
}

ListRequestsResult listRequests(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const ListRequestsQuery &query)
{
    if (!dbManager || !dbManager->getSession())
    {
        return failList(500, "数据库不可用");
    }
    if (query.operatorUserId <= 0)
    {
        return failList(400, "用户无效", "INVALID_USER");
    }
    if (!operatorHasApprove(dbManager, query.operatorUserId))
    {
        return failList(403, "缺少任职审批权限", "APPROVE_DENIED");
    }

    // status 白名单 fail-closed
    if (!query.status.empty() &&
        query.status != "pending" &&
        query.status != "approved" &&
        query.status != "rejected" &&
        query.status != "effective" &&
        query.status != "cancelled")
    {
        return failList(400, "status 取值不合法", "INVALID_STATUS");
    }

    const int page = std::max(1, query.page);
    const int pageSize = std::min(100, std::max(1, query.pageSize));
    const long long offset = static_cast<long long>(page - 1) * pageSize;

    const RbacService::EffectiveOrgScope scope =
        RbacService::loadEffectiveOrgScope(dbManager, query.operatorUserId);

    ListRequestsResult result;
    result.ok = true;
    result.httpStatus = 200;
    result.page = page;
    result.pageSize = pageSize;
    result.items = nlohmann::json::array();

    try
    {
        auto session = dbManager->getSession();
        std::string where = " WHERE 1 = 1 ";
        if (!query.status.empty())
        {
            where += " AND ea.status = ? ";
        }
        if (!scope.unrestricted)
        {
            if (scope.departmentIds.empty())
            {
                result.total = 0;
                return result;
            }
            where += " AND ea.department_id IN (" + joinDepartmentIds(scope.departmentIds) + ") ";
        }

        // 计数
        {
            std::string countSql =
                "SELECT COUNT(*) FROM employment_assignment ea " + where;
            auto stmt = session->sql(countSql);
            if (!query.status.empty())
            {
                stmt.bind(query.status);
            }
            mysqlx::Row countRow = stmt.execute().fetchOne();
            result.total = countRow && !countRow[0].isNull() ? countRow[0].get<int>() : 0;
        }

        // 展示名由快照 ID LEFT JOIN 解析；授权/范围仍只用 ea.department_id 快照。
        std::string listSql =
            "SELECT ea.id, ea.employment_id, e.user_id, "
            "COALESCE(u.name, ''), ea.branch_id, ea.department_id, "
            "ea.from_position_id, ea.to_position_id, ea.action, ea.status, "
            "ea.effective_from, ea.reason, ea.requested_by, ea.reviewed_by, "
            "ea.expected_employment_row_version, ea.row_version, ea.created_at, "
            "e.status, COALESCE(u.position_id, 0), "
            "COALESCE(b.name, ''), COALESCE(d.name, ''), "
            "COALESCE(fp.name, ''), COALESCE(tp.name, '') "
            "FROM employment_assignment ea "
            "JOIN employment e ON e.id = ea.employment_id "
            "JOIN users u ON u.id = e.user_id "
            "LEFT JOIN branches b ON b.id = ea.branch_id "
            "LEFT JOIN departments d ON d.id = ea.department_id "
            "LEFT JOIN positions fp ON fp.id = ea.from_position_id "
            "LEFT JOIN positions tp ON tp.id = ea.to_position_id " +
            where +
            " ORDER BY ea.created_at DESC, ea.id DESC "
            "LIMIT ? OFFSET ?";

        auto stmt = session->sql(listSql);
        if (!query.status.empty())
        {
            stmt.bind(query.status);
        }
        stmt.bind(pageSize, offset);
        mysqlx::SqlResult rows = stmt.execute();
        for (mysqlx::Row row = rows.fetchOne(); row; row = rows.fetchOne())
        {
            nlohmann::json item = {
                {"id", row[0].get<int64_t>()},
                {"employment_id", row[1].get<int64_t>()},
                {"user_id", row[2].get<int>()},
                {"user_name", row[3].isNull() ? "" : row[3].get<std::string>()},
                {"branch_id", row[4].isNull() ? 0 : row[4].get<int>()},
                {"department_id", row[5].isNull() ? 0 : row[5].get<int>()},
                {"from_position_id", row[6].isNull() ? nlohmann::json(nullptr) : nlohmann::json(row[6].get<int>())},
                {"to_position_id", row[7].isNull() ? nlohmann::json(nullptr) : nlohmann::json(row[7].get<int>())},
                {"action", row[8].isNull() ? "" : row[8].get<std::string>()},
                {"status", row[9].isNull() ? "" : row[9].get<std::string>()},
                {"effective_from", row[10].isNull() ? "" : row[10].get<std::string>()},
                {"reason", row[11].isNull() ? "" : row[11].get<std::string>()},
                {"requested_by", row[12].isNull() ? nlohmann::json(nullptr) : nlohmann::json(row[12].get<int>())},
                {"reviewed_by", row[13].isNull() ? nlohmann::json(nullptr) : nlohmann::json(row[13].get<int>())},
                {"expected_employment_row_version", row[14].isNull() ? 1 : row[14].get<int>()},
                {"row_version", row[15].isNull() ? 1 : row[15].get<int>()},
                {"created_at", row[16].isNull() ? "" : row[16].get<std::string>()},
                {"employment_status", row[17].isNull() ? "" : row[17].get<std::string>()},
                {"current_position_id", row[18].isNull() ? 0 : row[18].get<int>()},
                {"branch_name", row[19].isNull() ? "" : row[19].get<std::string>()},
                {"department_name", row[20].isNull() ? "" : row[20].get<std::string>()},
                {"from_position_name", row[21].isNull() ? "" : row[21].get<std::string>()},
                {"to_position_name", row[22].isNull() ? "" : row[22].get<std::string>()},
            };
            result.items.push_back(std::move(item));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "EmploymentAssignmentService::listRequests failed: " << e.what() << std::endl;
        return failList(500, std::string("查询失败: ") + e.what());
    }

    return result;
}
}
