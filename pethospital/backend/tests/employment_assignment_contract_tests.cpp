#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef EMPLOYMENT_ASSIGNMENT_SERVICE_SOURCE_PATH
#error "EMPLOYMENT_ASSIGNMENT_SERVICE_SOURCE_PATH is required"
#endif
#ifndef POSITION_PERMISSION_SERVICE_SOURCE_PATH
#error "POSITION_PERMISSION_SERVICE_SOURCE_PATH is required"
#endif
#ifndef PERSONNEL_HANDLER_SOURCE_PATH
#error "PERSONNEL_HANDLER_SOURCE_PATH is required"
#endif
#ifndef PERSONNEL_ROUTES_SOURCE_PATH
#error "PERSONNEL_ROUTES_SOURCE_PATH is required"
#endif
#ifndef PERSONNEL_ACCESS_SOURCE_PATH
#error "PERSONNEL_ACCESS_SOURCE_PATH is required"
#endif
#ifndef EMPLOYMENT_OUTBOX_SOURCE_PATH
#error "EMPLOYMENT_OUTBOX_SOURCE_PATH is required"
#endif
#ifndef DATABASE_MIGRATIONS_SOURCE_PATH
#error "DATABASE_MIGRATIONS_SOURCE_PATH is required"
#endif
#ifndef BOSS_ROUTES_SOURCE_PATH
#error "BOSS_ROUTES_SOURCE_PATH is required"
#endif
#ifndef BOSS_HANDLER_SOURCE_PATH
#error "BOSS_HANDLER_SOURCE_PATH is required"
#endif
#ifndef ADMIN_ROUTES_SOURCE_PATH
#error "ADMIN_ROUTES_SOURCE_PATH is required"
#endif

namespace
{
std::string readFile(const char *path)
{
    std::ifstream file(path);
    assert(file.is_open());
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void contains(const std::string &source, const std::string &value)
{
    assert(source.find(value) != std::string::npos);
}

void notContains(const std::string &source, const std::string &value)
{
    assert(source.find(value) == std::string::npos);
}

// 提取函数体，便于对状态机/break-glass 做结构断言，而非裸字符串存在性。
std::string extractFunction(const std::string &source, const std::string &signatureNeedle)
{
    const auto sigPos = source.find(signatureNeedle);
    assert(sigPos != std::string::npos);
    const auto bracePos = source.find('{', sigPos);
    assert(bracePos != std::string::npos);
    int depth = 0;
    for (std::size_t i = bracePos; i < source.size(); ++i)
    {
        if (source[i] == '{')
        {
            ++depth;
        }
        else if (source[i] == '}')
        {
            --depth;
            if (depth == 0)
            {
                return source.substr(bracePos, i - bracePos + 1);
            }
        }
    }
    assert(false && "unbalanced braces in extractFunction");
    return {};
}
}

int main()
{
    const std::string service = readFile(EMPLOYMENT_ASSIGNMENT_SERVICE_SOURCE_PATH);
    const std::string positionService = readFile(POSITION_PERMISSION_SERVICE_SOURCE_PATH);
    const std::string personnelHandler = readFile(PERSONNEL_HANDLER_SOURCE_PATH);
    const std::string personnelRoutes = readFile(PERSONNEL_ROUTES_SOURCE_PATH);
    const std::string personnelAccess = readFile(PERSONNEL_ACCESS_SOURCE_PATH);
    const std::string outbox = readFile(EMPLOYMENT_OUTBOX_SOURCE_PATH);
    const std::string migrations = readFile(DATABASE_MIGRATIONS_SOURCE_PATH);
    const std::string bossRoutes = readFile(BOSS_ROUTES_SOURCE_PATH);
    const std::string bossHandler = readFile(BOSS_HANDLER_SOURCE_PATH);
    const std::string adminRoutes = readFile(ADMIN_ROUTES_SOURCE_PATH);

    // B2/B6: 职位权限事务内原始行 floor
    contains(positionService, "FOR UPDATE");
    contains(positionService, "SELECT permission_key FROM position_permissions WHERE position_id = ?");
    notContains(positionService, "RbacService::loadPermissionsForPosition");

    // B6: assignment gate = max(stored policy, raw-key floor)
    contains(service, "loadRawPermissionKeysUnderLock");
    contains(service, "effectivePolicyGate");
    contains(service, "requiredAssignmentPolicy");
    contains(service, "maxAssignmentPolicy");

    // B3/B5/B10: employment CAS affected==1
    contains(service, "FROM users WHERE id = ? LIMIT 1 FOR UPDATE");
    contains(service, "FROM employment WHERE user_id = ? LIMIT 1 FOR UPDATE");
    contains(service, "INSERT INTO employment_assignment");
    contains(service, "INSERT INTO employment_event_outbox");
    contains(service, "getAffectedItemsCount() != 1");
    contains(service, "STALE_VERSION");
    contains(service, "casBumpEmploymentRowVersion");

    // B4
    contains(service, "Action::Offboard");
    contains(service, "Action::Regularize");
    contains(service, "ApprovalRequired");
    contains(service, "SELF_ASSIGNMENT");

    // B7/B16: action 必须显式；精确权限；禁止字段推断
    contains(personnelRoutes, "PersonnelAccess::parseAssignmentAction");
    contains(personnelRoutes, "PersonnelAccess::canPerformAssignmentAction");
    contains(personnelRoutes, "action 必填");
    contains(personnelRoutes, "regularize");
    notContains(personnelRoutes, "expected > 0 ? \"transfer\" : \"onboard\"");
    notContains(personnelRoutes, "resolved = \"offboard\"");
    contains(personnelHandler, "action 必填");
    notContains(personnelHandler, "action == \"onboard\" || action.empty()");
    contains(personnelAccess, "kEmploymentOnboard");
    contains(personnelAccess, "kEmploymentAssign");
    contains(personnelAccess, "kEmploymentOffboard");
    contains(personnelAccess, "kEmploymentRegularize");
    contains(personnelAccess, "operatorHoldsPersonnelDomainPosition");
    contains(personnelAccess, "staff_kind = 'personnel'");
    contains(personnelAccess, "business_domain");
    contains(personnelAccess, "kStaffRoleWrite");

    // B8: staff-role:write 仅人事域
    contains(personnelAccess, "operatorHoldsPersonnelDomainPosition");
    notContains(personnelRoutes, "hasPersonnelEmploymentAccess");

    // B9 seed 纪律
    notContains(migrations, "UNION ALL SELECT 'president', 'employment:onboard'");
    notContains(migrations, "UNION ALL SELECT 'vice-president', 'employment:assign'");
    notContains(migrations, "UNION ALL SELECT 'Boss', 'employment:onboard'");
    notContains(migrations, "UNION ALL SELECT 'Boss', 'employment:offboard'");
    contains(migrations, "UNION ALL SELECT 'president', 'employment-assignment:approve'");
    contains(migrations, "UNION ALL SELECT 'Boss', 'employment-assignment:approve'");
    contains(migrations, "staff_kind = 'personnel' OR COALESCE(d.business_domain, '') = 'personnel'");
    contains(migrations, "staff-role:write");
    contains(migrations, "salary-profile:activate");
    notContains(migrations, "UNION ALL SELECT 'super-admin', 'salary-profile:activate'");
    notContains(migrations, "UNION ALL SELECT 'super-admin', 'compensation:approve'");

    // B17: 迁移安全下限用 catalog，无硬编码 permission_key IN 安全清单
    contains(migrations, "Permissions::requiredAssignmentPolicy");
    contains(migrations, "rawKeys");
    notContains(migrations, "pp.permission_key IN (");
    // B18: 完整消费 positions 后再嵌套查询
    contains(migrations, "std::vector<PositionPolicyRow> positionRows");
    contains(migrations, "positions SqlResult 已离开作用域");

    // B11 outbox 租约
    contains(outbox, "locked_at");
    contains(outbox, "lease_expired");
    contains(outbox, "kProcessingLeaseSeconds");
    contains(outbox, "status = 'processing'");
    contains(migrations, "locked_at DATETIME NULL");

    // B12 受限 customer fail-closed
    contains(personnelHandler, "AND 1 = 0");
    contains(personnelHandler, "u.account_type = 'staff' AND pos.department_id IN");
    notContains(personnelHandler, "u.account_type = 'customer' OR pos.department_id");

    notContains(personnelRoutes, "doctor-assignments");
    notContains(personnelHandler, "createDoctor");

    // ---- B1: 公开 API 与权威 v6 一致；禁止双契约 / 旧路径 ----
    contains(personnelRoutes, "/api/personnel/employees/<int>/regularization");
    contains(personnelRoutes, "/api/personnel/employees/<int>/offboarding");
    notContains(personnelRoutes, "regularization-requests");
    notContains(personnelRoutes, "offboarding-requests");
    contains(personnelRoutes, "kEmploymentRegularize");
    contains(personnelRoutes, "kEmploymentOffboard");
    contains(personnelRoutes, "createRegularization");
    contains(personnelRoutes, "createOffboarding");
    notContains(personnelRoutes, "createRegularizationRequest");
    notContains(personnelRoutes, "createOffboardingRequest");
    notContains(personnelHandler, "createRegularizationRequest");
    notContains(personnelHandler, "createOffboardingRequest");
    contains(personnelHandler, "createRegularization");
    contains(personnelHandler, "createOffboarding");

    // Boss：复用复数 bosses 模块 + approvals 命名
    contains(bossRoutes, "/api/bosses/employment-assignment-approvals");
    contains(bossRoutes, "/api/bosses/employment-assignment-approvals/<int>/decision");
    notContains(bossRoutes, "/api/boss/employment-assignment-requests");
    notContains(bossRoutes, "employment-assignment-requests");
    contains(bossRoutes, "kEmploymentAssignmentApprove");
    contains(bossRoutes, "listEmploymentAssignmentApprovals");
    contains(bossRoutes, "decideEmploymentAssignmentApproval");
    notContains(bossRoutes, "listEmploymentAssignmentRequests");
    notContains(bossRoutes, "decideEmploymentAssignmentRequest");
    contains(bossHandler, "listEmploymentAssignmentApprovals");
    contains(bossHandler, "decideEmploymentAssignmentApproval");
    notContains(bossHandler, "listEmploymentAssignmentRequests");
    notContains(bossHandler, "decideEmploymentAssignmentRequest");
    // 精确权限，不用通用 management 门槛
    notContains(bossRoutes, "isValidManagementToken");
    notContains(bossRoutes, "isValidBossPortalToken");
    contains(bossRoutes, "isValidPermissionToken");

    // decision body 显式 action + reason + expectedRowVersion
    contains(bossHandler, "action 必填");
    contains(bossHandler, "expectedRowVersion 必填");
    contains(bossHandler, "reason 不能为空");
    contains(bossHandler, "\"approve\"");
    contains(bossHandler, "\"reject\"");
    contains(bossHandler, "action 取值不合法");
    contains(bossHandler, "DecisionAction::Approve");
    contains(bossHandler, "DecisionAction::Reject");
    // 列表统一 {items,total,page,pageSize}
    contains(bossHandler, "{\"items\"");
    contains(bossHandler, "{\"total\"");
    contains(bossHandler, "{\"page\"");
    contains(bossHandler, "{\"pageSize\"");

    // 服务：decide + 双 CAS + 自审禁止 + 组织范围
    contains(service, "DecisionResult decide");
    contains(service, "ListRequestsResult listRequests");
    contains(service, "kEmploymentAssignmentApprove");
    contains(service, "SELF_APPROVAL");
    contains(service, "requestedBy == request.operatorUserId");
    contains(service, "ALREADY_DECIDED");
    contains(service, "status = 'pending' AND row_version = ?");
    contains(service, "expected_employment_row_version");
    contains(service, "employmentRowVersion != expectedEmpRowVersion");
    contains(service, "departmentInScope");
    contains(service, "assignment_approved");
    contains(service, "assignment_rejected");
    contains(service, "assignment_break_glass_effective");

    // 审批列表展示字段：快照 org/职位名；范围仍用 ea.department_id
    {
        const std::string listFn =
            extractFunction(service, "ListRequestsResult listRequests");
        contains(listFn, "\"branch_name\"");
        contains(listFn, "\"department_name\"");
        contains(listFn, "\"from_position_name\"");
        contains(listFn, "\"to_position_name\"");
        contains(listFn, "LEFT JOIN branches b ON b.id = ea.branch_id");
        contains(listFn, "LEFT JOIN departments d ON d.id = ea.department_id");
        contains(listFn, "LEFT JOIN positions fp ON fp.id = ea.from_position_id");
        contains(listFn, "LEFT JOIN positions tp ON tp.id = ea.to_position_id");
        contains(listFn, "ea.department_id IN (");
    }

    // ---- B2: 生命周期状态机（函数体结构断言，不只是存在性）----
    const std::string regularizableFn =
        extractFunction(service, "bool isRegularizableEmployment");
    // 仅 probation 可转正
    contains(regularizableFn, "probation");
    notContains(regularizableFn, "onboarding");
    notContains(regularizableFn, "probationWaived");
    notContains(regularizableFn, "probation_waived");

    const std::string offboardableFn =
        extractFunction(service, "bool isOffboardableEmployment");
    contains(offboardableFn, "probation");
    contains(offboardableFn, "regularization_pending");
    contains(offboardableFn, "active");
    // 不允许 onboarding 当离职
    notContains(offboardableFn, "onboarding");

    // onboarding reject：CAS employment.status='rejected'；其他 reject 不改 employment
    const std::string decideFn = extractFunction(service, "DecisionResult decide");
    contains(decideFn, "Action::Onboard");
    contains(decideFn, "status = 'rejected'");
    contains(decideFn, "UPDATE employment SET status = 'rejected'");
    contains(decideFn, "row_version = row_version + 1");
    contains(decideFn, "employmentRowVersion != expectedEmpRowVersion");
    contains(decideFn, "employment_rejected");
    // 仅 onboard reject 分支写 employment rejected（guarded by action check）
    contains(decideFn, "if (action == Action::Onboard)");

    // ---- B3: user_permissions 仅 onboard/transfer/offboard 清理 ----
    const std::string applyFn = extractFunction(service, "ApplyOutcome applyEffectiveUnderLock");
    contains(applyFn, "DELETE FROM user_permissions WHERE user_id = ?");
    contains(applyFn, "Action::Onboard");
    contains(applyFn, "Action::Transfer");
    contains(applyFn, "Action::Offboard");
    // 不得在 Regularize 分支无条件删权限：删除必须在 action 三选一守卫内
    {
        const auto delPos = applyFn.find("DELETE FROM user_permissions WHERE user_id = ?");
        assert(delPos != std::string::npos);
        // 向前找最近的 if 守卫
        const auto guardWindow = applyFn.substr(delPos > 400 ? delPos - 400 : 0, 400);
        contains(guardWindow, "Action::Onboard");
        contains(guardWindow, "Action::Transfer");
        contains(guardWindow, "Action::Offboard");
        notContains(guardWindow, "Action::Regularize");
    }

    // ---- B4: break-glass 真实且显式 ----
    const std::string assignFn = extractFunction(service, "AssignResult assign");
    // 新条件：explicitBreakGlass && holdsRbacManage（全 action）
    contains(assignFn, "request.explicitBreakGlass && holdsRbacManage");
    contains(assignFn, "BREAK_GLASS_DENIED");
    contains(assignFn, "explicitBreakGlass");
    // 旧条件：仅 offboard/regularize 才要 explicit，或 onboard/transfer 持 manage 静默 true
    notContains(assignFn, "((isOffboard || isRegularize) ? request.explicitBreakGlass : true)");
    notContains(assignFn, "(isOffboard || isRegularize) && request.explicitBreakGlass && !holdsRbacManage");
    // makeEffective 分支不得无条件 breakGlass=true
    notContains(assignFn, "applyCtx.breakGlass = true;");
    contains(assignFn, "applyCtx.breakGlass = breakGlass;");
    // 审计：break-glass → assignment_break_glass_effective；普通 → assignment_effective
    contains(assignFn, "assignment_break_glass_effective");
    contains(assignFn, "assignment_effective");
    contains(assignFn, "breakGlass ? \"assignment_break_glass_effective\" : \"assignment_effective\"");
    // 旧：仅 offboard/regularize break-glass 才写 break_glass audit
    notContains(assignFn, "breakGlass && (isOffboard || isRegularize)");
    // after_snapshot 使用真实 breakGlass
    contains(assignFn, "afterSnap[\"break_glass\"] = breakGlass");
    notContains(assignFn, "afterSnap[\"break_glass\"] = true");

    // 普通 personnel 派岗不设 explicitBreakGlass；admin 显式 true
    const std::string personnelAssignHandler =
        extractFunction(personnelHandler, "personnelHandler::updateEmployeeAssignment");
    notContains(personnelAssignHandler, "explicitBreakGlass = true");
    contains(adminRoutes, "explicitBreakGlass = true");
    // 未显式时不绕过 approval_required：makeEffective 仅 breakGlass 或 personnel_direct
    contains(assignFn, "Permissions::AssignmentPolicy::PersonnelDirect");
    contains(assignFn, "breakGlass ||");
    contains(assignFn, "gate == Permissions::AssignmentPolicy::PersonnelDirect");

    // ---- B5: side_effects_required 显式布尔；dispatcher 只认该字段 ----
    contains(service, "side_effects_required");
    contains(service, "{\"side_effects_required\", false}");
    contains(service, "{\"side_effects_required\", true}");
    // apply effective payload true
    contains(applyFn, "side_effects_required");
    contains(applyFn, "true");
    // pending/reject false
    contains(assignFn, "side_effects_required");
    contains(decideFn, "side_effects_required");

    // dispatcher：缺字段默认 true（兼容升级前有效 payload）；禁止 false 默认
    contains(outbox, "side_effects_required");
    contains(outbox, "payload.value(\"side_effects_required\", true)");
    notContains(outbox, "payload.value(\"side_effects_required\", false)");
    notContains(outbox, "assignmentStatus == \"pending\"");
    notContains(outbox, "decision == \"reject\"");
    notContains(outbox, "payload.value(\"assignment_status\"");
    notContains(outbox, "payload.value(\"decision\"");

    // 转正/离职最低审批
    contains(service, "isRegularize");
    contains(service, "isOffboard || isRegularize");
    contains(service, "isRegularizableEmployment");
    contains(service, "isOffboardableEmployment");
    contains(service, "regularization_pending");
    contains(personnelHandler, "break_glass");
    contains(personnelHandler, "Action::Regularize");
    contains(personnelHandler, "Action::Offboard");

    // 审计/outbox 同事务（commit 前 INSERT audit + outbox）
    contains(service, "insertWorkflowAudit");
    contains(service, "INSERT INTO employment_event_outbox");
    contains(service, "COMMIT");
    // 生效路径不在 handler 直写 users
    notContains(personnelHandler, "UPDATE users SET account_type");
    notContains(personnelHandler, "UPDATE users SET position_id");
    notContains(bossHandler, "UPDATE users SET account_type");
    notContains(bossHandler, "UPDATE users SET position_id");
    notContains(personnelRoutes, "UPDATE users SET");
    notContains(bossRoutes, "UPDATE users SET");
    // 服务是唯一 users 任职写入口
    contains(service, "UPDATE users SET account_type = 'customer', position_id = NULL");
    contains(service, "UPDATE users SET account_type = 'staff', position_id = ?");

    // onboarding approval_required 经同一 decide 接口
    contains(service, "Action::Onboard");
    contains(service, "parseActionKey");
    contains(service, "applyEffectiveUnderLock");

    // 未知动作 fail closed
    contains(service, "UNKNOWN_ACTION");
    contains(bossHandler, "action 取值不合法");

    // 禁止恢复旧 doctor/warehouse 硬编码接口
    notContains(personnelRoutes, "warehouse-manager-assignments");
    notContains(personnelRoutes, "doctor-removals");

    return 0;
}
