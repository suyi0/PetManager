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
AssignResult fail(int httpStatus, const std::string &message, const std::string &code = "")
{
    AssignResult result;
    result.ok = false;
    result.httpStatus = httpStatus;
    result.message = message;
    result.errorCode = code;
    return result;
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

bool targetHoldsRbacManage(
    mysqlx::Session &session,
    int targetUserId,
    const std::string &systemKey)
{
    if (systemKey == "super-admin")
    {
        return true;
    }
    // 在事务内用原始权限行判断（职位 + 个人），不经过过滤未知 key 的展示路径。
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
    case Action::Offboard:
        return "offboard";
    }
    return "transfer";
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
        session.sql("INSERT INTO onlineDoctors (doctor_id, date, check_in_time, check_out_time, status) "
                    "VALUES (?, ?, NULL, NULL, 'offline')")
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
                                      "SELECT id, status, row_version FROM employment WHERE user_id = ? LIMIT 1 FOR UPDATE")
                               .bind(userId)
                               .execute()
                               .fetchOne();
    if (existing)
    {
        LockedEmployment emp;
        emp.id = existing[0].get<int64_t>();
        emp.status = existing[1].isNull() ? "draft" : existing[1].get<std::string>();
        emp.rowVersion = existing[2].isNull() ? 1 : existing[2].get<int>();
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
                                     "SELECT id, status, row_version FROM employment WHERE user_id = ? LIMIT 1 FOR UPDATE")
                              .bind(userId)
                              .execute()
                              .fetchOne();
    LockedEmployment emp;
    emp.id = created[0].get<int64_t>();
    emp.status = created[1].isNull() ? "draft" : created[1].get<std::string>();
    emp.rowVersion = created[2].isNull() ? 1 : created[2].get<int>();
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
}

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
    if (!isOffboard)
    {
        if (!request.targetPositionId.has_value() || request.targetPositionId.value() <= 0)
        {
            return fail(400, "目标职位不能为空", "POSITION_REQUIRED");
        }
    }

    // 组织范围只读；职位策略权威在事务内锁后读取。
    const RbacService::EffectiveOrgScope scope =
        RbacService::loadEffectiveOrgScope(dbManager, request.operatorUserId);
    const bool breakGlass = operatorHasRbacManage(dbManager, request.operatorUserId);
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
        if (!isOffboard)
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
        if (request.action == Action::Offboard && positionId <= 0)
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
        // offboard 始终至少 approval_required（再与 floor 取 max）。
        Permissions::AssignmentPolicy gate = Permissions::AssignmentPolicy::PersonnelDirect;
        if (isOffboard)
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

        // 非 break-glass：approval_required / offboard / super_admin_only → pending；
        // 仅 gate==personnel_direct 的 onboard/transfer 可直接 effective。
        const bool makeEffective =
            breakGlass ||
            (!isOffboard && gate == Permissions::AssignmentPolicy::PersonnelDirect);

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
            isOffboard ? "" : (targetPosition.has_value() ? targetPosition->staffKind : "");
        const std::string assignmentStatus = makeEffective ? "effective" : "pending";

        nlohmann::json beforeSnap = {
            {"user_id", userId},
            {"account_type", accountType},
            {"position_id", positionId},
            {"employment_status", employment.status},
            {"employment_row_version", employment.rowVersion},
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
        afterSnap["to_position_id"] = isOffboard ? nlohmann::json(nullptr) : nlohmann::json(request.targetPositionId.value());
        afterSnap["break_glass"] = breakGlass;

        if (makeEffective)
        {
            if (isOffboard)
            {
                session->sql("UPDATE users SET account_type = 'customer', position_id = NULL WHERE id = ?")
                    .bind(userId)
                    .execute();
                auto empUpd = session->sql(
                                         "UPDATE employment SET status = 'separated', separated_at = NOW(), "
                                         "row_version = row_version + 1 WHERE id = ? AND row_version = ?")
                                  .bind(employment.id, employment.rowVersion)
                                  .execute();
                if (empUpd.getAffectedItemsCount() != 1)
                {
                    rollbackTransactionQuietly(*session);
                    return fail(409, "员工任职版本已变更，请刷新后重试", "STALE_VERSION");
                }
                afterSnap["account_type"] = "customer";
                afterSnap["position_id"] = 0;
                afterSnap["employment_status"] = "separated";
            }
            else
            {
                session->sql("UPDATE users SET account_type = 'staff', position_id = ? WHERE id = ?")
                    .bind(request.targetPositionId.value(), userId)
                    .execute();
                const std::string nextEmpStatus =
                    (employment.status == "draft" || employment.status == "rejected" || employment.status == "separated")
                        ? "onboarding"
                        : employment.status;
                if (request.action == Action::Onboard)
                {
                    auto empUpd = session->sql(
                                             "UPDATE employment SET status = 'onboarding', hire_date = COALESCE(hire_date, ?), "
                                             "row_version = row_version + 1 WHERE id = ? AND row_version = ?")
                                      .bind(effectiveFrom, employment.id, employment.rowVersion)
                                      .execute();
                    if (empUpd.getAffectedItemsCount() != 1)
                    {
                        rollbackTransactionQuietly(*session);
                        return fail(409, "员工任职版本已变更，请刷新后重试", "STALE_VERSION");
                    }
                    afterSnap["employment_status"] = "onboarding";
                }
                else
                {
                    if (!casBumpEmploymentRowVersion(*session, employment.id, employment.rowVersion))
                    {
                        rollbackTransactionQuietly(*session);
                        return fail(409, "员工任职版本已变更，请刷新后重试", "STALE_VERSION");
                    }
                    afterSnap["employment_status"] = nextEmpStatus;
                }
                afterSnap["account_type"] = "staff";
                afterSnap["position_id"] = request.targetPositionId.value();
                if (newStaffKind == "doctor")
                {
                    ensureDoctorOnlineRow(*session, userId);
                }
            }

            // 清空个人例外权限
            session->sql("DELETE FROM user_permissions WHERE user_id = ?")
                .bind(userId)
                .execute();

            // outbox
            nlohmann::json payload = {
                {"assignment_id", assignmentId},
                {"action", actionKey(request.action)},
                {"account_type", isOffboard ? "customer" : "staff"},
                {"old_position_id", positionId},
                {"new_position_id", isOffboard ? 0 : request.targetPositionId.value()},
                {"old_staff_kind", oldStaffKind},
                {"new_staff_kind", newStaffKind},
            };
            const std::string eventType = isOffboard ? "employment_separated" : "assignment_changed";
            const std::string eventKey =
                "assignment:" + std::to_string(assignmentId) + ":" + eventType;
            auto outboxIns = session->sql(
                                        "INSERT INTO employment_event_outbox "
                                        "(event_key, employment_id, user_id, event_type, payload, status) "
                                        "VALUES (?, ?, ?, ?, ?, 'pending')")
                                 .bind(eventKey, employment.id, userId, eventType, payload.dump())
                                 .execute();
            outboxId = static_cast<long long>(outboxIns.getAutoIncrementValue());
            afterSnap["outbox_id"] = outboxId;
        }

        insertWorkflowAudit(
            *session,
            "assignment",
            assignmentId,
            makeEffective ? "assignment_effective" : "assignment_pending",
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
                                  ? (isOffboard ? 0 : request.targetPositionId.value())
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
        // open_slot 唯一约束冲突
        const std::string what = e.what();
        if (what.find("Duplicate") != std::string::npos || what.find("uq_employment_assignment_open") != std::string::npos)
        {
            return fail(409, "已有进行中的任职申请", "OPEN_ASSIGNMENT_EXISTS");
        }
        return fail(500, std::string("任职变更失败: ") + e.what());
    }

    // 提交后以 outbox 为权威消费副作用
    if (okResult.ok && outboxId > 0)
    {
        EmploymentOutboxDispatcher::dispatchOne(dbManager, outboxId);
    }
    return okResult;
}
}
