#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef COMPENSATION_WORKFLOW_SERVICE_SOURCE_PATH
#error "COMPENSATION_WORKFLOW_SERVICE_SOURCE_PATH is required"
#endif
#ifndef DATABASE_MIGRATIONS_SOURCE_PATH
#error "DATABASE_MIGRATIONS_SOURCE_PATH is required"
#endif
#ifndef PERSONNEL_ROUTES_SOURCE_PATH
#error "PERSONNEL_ROUTES_SOURCE_PATH is required"
#endif
#ifndef PERSONNEL_HANDLER_SOURCE_PATH
#error "PERSONNEL_HANDLER_SOURCE_PATH is required"
#endif
#ifndef BOSS_ROUTES_SOURCE_PATH
#error "BOSS_ROUTES_SOURCE_PATH is required"
#endif
#ifndef BOSS_HANDLER_SOURCE_PATH
#error "BOSS_HANDLER_SOURCE_PATH is required"
#endif
#ifndef FINANCE_ROUTES_SOURCE_PATH
#error "FINANCE_ROUTES_SOURCE_PATH is required"
#endif
#ifndef FINANCE_HANDLER_SOURCE_PATH
#error "FINANCE_HANDLER_SOURCE_PATH is required"
#endif
#ifndef PERMISSIONS_SOURCE_PATH
#error "PERMISSIONS_SOURCE_PATH is required"
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
    const std::string service = readFile(COMPENSATION_WORKFLOW_SERVICE_SOURCE_PATH);
    const std::string migrations = readFile(DATABASE_MIGRATIONS_SOURCE_PATH);
    const std::string personnelRoutes = readFile(PERSONNEL_ROUTES_SOURCE_PATH);
    const std::string personnelHandler = readFile(PERSONNEL_HANDLER_SOURCE_PATH);
    const std::string bossRoutes = readFile(BOSS_ROUTES_SOURCE_PATH);
    const std::string bossHandler = readFile(BOSS_HANDLER_SOURCE_PATH);
    const std::string financeRoutes = readFile(FINANCE_ROUTES_SOURCE_PATH);
    const std::string financeHandler = readFile(FINANCE_HANDLER_SOURCE_PATH);
    const std::string permissions = readFile(PERMISSIONS_SOURCE_PATH);

    // ---- schema ----
    contains(migrations, "\"compensation_proposal\"");
    contains(migrations, "CREATE TABLE compensation_proposal");
    contains(migrations, "UNIQUE KEY uq_compensation_proposal_open (employment_id, phase, open_slot)");
    contains(migrations, "CONSTRAINT chk_cp_pay_basis CHECK");
    contains(migrations, "CONSTRAINT chk_cp_sod_propose_approve CHECK");
    contains(migrations, "CONSTRAINT chk_cp_sod_approve_finance CHECK");
    contains(migrations, "CONSTRAINT chk_cp_sod_propose_finance CHECK");
    contains(migrations, "CONSTRAINT fk_cp_approved_by FOREIGN KEY (approved_by) REFERENCES users(id),");
    contains(migrations, "CONSTRAINT fk_cp_finance_by FOREIGN KEY (finance_confirmed_by) REFERENCES users(id),");
    notContains(migrations, "fk_cp_approved_by FOREIGN KEY (approved_by) REFERENCES users(id) ON DELETE SET NULL");
    notContains(migrations, "fk_cp_finance_by FOREIGN KEY (finance_confirmed_by) REFERENCES users(id) ON DELETE SET NULL");
    contains(migrations, "open_slot TINYINT GENERATED ALWAYS AS");
    // v6 open_slot 含 finance_confirmed，与 isOpenStatus 对齐
    contains(migrations,
             "'draft','submitted','management_approved','returned','finance_confirmed'");
    contains(migrations, "probation','regular','adjustment");
    // 建表顺序契约：salaryProfile 必须先于 compensation_proposal（FK 依赖），
    // 不能只证明两个字符串都存在。
    {
        const auto salaryCreatePos = migrations.find("CREATE TABLE salaryProfile");
        const auto proposalCreatePos = migrations.find("CREATE TABLE compensation_proposal");
        assert(salaryCreatePos != std::string::npos);
        assert(proposalCreatePos != std::string::npos);
        assert(salaryCreatePos < proposalCreatePos);
        // FK 仍指向 salaryProfile
        contains(migrations,
                 "CONSTRAINT fk_cp_salary_profile FOREIGN KEY (salary_profile_id) "
                 "REFERENCES salaryProfile(id)");
        // 增量 onExists 对齐 open_slot（幂等）
        contains(migrations, "compensation_proposal.open_slot aligned");
        contains(migrations, "GENERATION_EXPRESSION");
        // 原子 ALTER 契约：预检重复 + 单条 ALTER 含 DROP/ADD；禁止旧式分步 DROP 与 catch-and-skip
        contains(migrations, "duplicate open");
        contains(migrations, "open-status set");
        contains(migrations, "DROP INDEX uq_compensation_proposal_open,");
        contains(migrations, "DROP COLUMN open_slot,");
        contains(migrations, "ADD UNIQUE KEY uq_compensation_proposal_open");
        contains(migrations, "resolve before upgrade");
        notContains(migrations, "align skipped");
        notContains(migrations, "open_slot align skipped");
        // 旧路径：单独 DROP INDEX / DROP COLUMN 作为独立 SQL 字符串（以 ") 结尾，无同句逗号）
        notContains(migrations, "DROP INDEX uq_compensation_proposal_open\")");
        notContains(migrations, "DROP COLUMN open_slot\")");
        notContains(migrations, "ADD UNIQUE KEY uq_compensation_proposal_open "
                                "(employment_id, phase, open_slot)\")");
    }
    // 不改写存量 salaryProfile 金额；无 backfill 改 base_salary
    notContains(migrations, "UPDATE salaryProfile SET base_salary");
    notContains(migrations, "UPDATE salaryProfile SET hourly_rate");
    // super-admin 不默认 compensation:approve / salary-profile:activate
    notContains(migrations, "UNION ALL SELECT 'super-admin', 'compensation:approve'");
    notContains(migrations, "UNION ALL SELECT 'super-admin', 'salary-profile:activate'");
    contains(migrations, "UNION ALL SELECT 'president', 'compensation:approve'");
    contains(migrations, "UNION ALL SELECT 'finance-director', 'salary-profile:activate'");
    contains(migrations, "UNION ALL SELECT 'Personnel', 'compensation:propose'");
    contains(migrations, "UNION ALL SELECT 'Personnel', 'compensation:reassign-case'");

    // ---- 权限目录（Permissions.cpp 引用 catalog 常量）----
    contains(permissions, "kCompensationPropose");
    contains(permissions, "kCompensationReassignCase");
    contains(permissions, "kCompensationApprove");
    contains(permissions, "kSalaryProfileActivate");

    // ---- 人事路由 ----
    contains(personnelRoutes, "/api/personnel/compensation-proposals");
    contains(personnelRoutes, "/api/personnel/compensation-proposals/<int>");
    contains(personnelRoutes, "/api/personnel/compensation-proposals/<int>/submit");
    contains(personnelRoutes, "/api/personnel/compensation-proposals/<int>/reassign");
    contains(personnelRoutes, "kCompensationPropose");
    contains(personnelRoutes, "kCompensationReassignCase");
    contains(personnelRoutes, "createCompensationProposal");
    contains(personnelRoutes, "reassignCompensationProposal");
    contains(personnelHandler, "listCompensationProposals");
    contains(personnelHandler, "createCompensationProposal");
    contains(personnelHandler, "updateCompensationProposal");
    contains(personnelHandler, "submitCompensationProposal");
    contains(personnelHandler, "reassignCompensationProposal");
    contains(personnelHandler, "{\"items\"");
    contains(personnelHandler, "{\"total\"");
    contains(personnelHandler, "{\"page\"");
    contains(personnelHandler, "{\"pageSize\"");
    // 普通员工搜索/详情：任职元数据契约 + 绝不增加薪资金额
    {
        const std::string searchFn =
            extractFunction(personnelHandler, "crow::response personnelHandler::searchEmployees");
        const std::string getFn =
            extractFunction(personnelHandler, "crow::response personnelHandler::getEmployee");
        // employment 1:1 join + 前端工作流所需元数据（无薪资）
        contains(searchFn, "LEFT JOIN employment e ON e.user_id = u.id");
        contains(searchFn, "\"employment_id\"");
        contains(searchFn, "\"employment_status\"");
        contains(searchFn, "\"employment_row_version\"");
        contains(searchFn, "\"hire_date\"");
        contains(searchFn, "\"probation_waived\"");
        contains(getFn, "LEFT JOIN employment e ON e.user_id = u.id");
        contains(getFn, "\"employment_id\"");
        contains(getFn, "\"employment_status\"");
        contains(getFn, "\"employment_row_version\"");
        contains(getFn, "\"hire_date\"");
        contains(getFn, "\"probation_waived\"");
        notContains(searchFn, "base_salary");
        notContains(searchFn, "hourly_rate");
        notContains(searchFn, "baseSalary");
        notContains(searchFn, "salaryProfile");
        notContains(getFn, "base_salary");
        notContains(getFn, "hourly_rate");
        notContains(getFn, "baseSalary");
        notContains(getFn, "salaryProfile");
    }

    // ---- Boss 路由 ----
    contains(bossRoutes, "/api/bosses/compensation-approvals");
    contains(bossRoutes, "/api/bosses/compensation-approvals/<int>/decision");
    contains(bossRoutes, "kCompensationApprove");
    contains(bossRoutes, "isValidPermissionToken");
    notContains(bossRoutes, "isValidManagementToken");
    contains(bossHandler, "listCompensationApprovals");
    contains(bossHandler, "decideCompensationApproval");
    contains(bossHandler, "action 必填");
    contains(bossHandler, "expectedRowVersion 必填");
    contains(bossHandler, "reason 不能为空");
    contains(bossHandler, "\"approve\"");
    contains(bossHandler, "\"return\"");
    contains(bossHandler, "action 取值不合法");

    // ---- Finance 路由 / 激活 ----
    contains(financeRoutes, "/api/finance/compensation-activations");
    contains(financeRoutes, "/api/finance/compensation-activations/<int>/confirm");
    contains(financeRoutes, "/api/finance/compensation-activations/<int>/return");
    contains(financeRoutes, "kSalaryProfileActivate");
    contains(financeHandler, "listCompensationActivations");
    contains(financeHandler, "confirmCompensationActivation");
    contains(financeHandler, "returnCompensationActivation");
    contains(financeHandler, "财务确认不允许修改金额字段");

    // ---- 旧 POST 旁路关闭 ----
    const std::string saveSalaryProfileFn =
        extractFunction(financeHandler, "crow::response financeHandler::saveSalaryProfile");
    contains(saveSalaryProfileFn, "410");
    contains(saveSalaryProfileFn, "SALARY_PROFILE_WRITE_RETIRED");
    notContains(saveSalaryProfileFn, "INSERT INTO salaryProfile");
    notContains(saveSalaryProfileFn, "ON DUPLICATE KEY UPDATE");
    // 路由 POST 不再要求 salary:write
    contains(financeRoutes, "const std::string permission = Permissions::kSalaryRead");
    notContains(financeRoutes, "isWrite ? Permissions::kSalaryWrite : Permissions::kSalaryRead");

    // ---- 服务：精确权限 / assignee / 金额 / 状态机 / 职责分离 / 双 CAS ----
    contains(service, "kCompensationPropose");
    contains(service, "kCompensationReassignCase");
    contains(service, "kCompensationApprove");
    contains(service, "kSalaryProfileActivate");
    contains(service, "NOT_ASSIGNEE");
    contains(service, "ASSIGNEE_INVALID");
    contains(service, "AMOUNT_FROZEN");
    contains(service, "OPEN_PROPOSAL_EXISTS");
    contains(service, "SELF_APPROVAL");
    contains(service, "SEPARATION_OF_DUTIES");
    contains(service, "STALE_VERSION");
    contains(service, "EMPLOYMENT_STATE_CONFLICT");
    contains(service, "EFFECTIVE_DATE_CONFLICT");
    contains(service, "expected_employment_row_version");
    contains(service, "management_approved");
    contains(service, "resource_type");
    contains(service, "'compensation'");
    contains(service, "uq_compensation_proposal_open");
    contains(service, "FOR UPDATE");
    contains(service, "getAffectedItemsCount() != 1");
    contains(service, "DATE_SUB(?, INTERVAL 1 DAY)");
    contains(service, "INSERT INTO salaryProfile");
    contains(service, "status = 'probation'");
    contains(service, "status = 'active'");
    contains(service, "regularization_pending");
    contains(service, "probation_waived");
    contains(service, "SALARY_PROFILE_OVERLAP");
    contains(service, "不能将已结束任职翻回在职");
    contains(service, "separated");
    contains(service, "rejected");
    contains(service, "finance_confirmed");
    contains(service, "isOpenStatus");
    contains(service, "kOpenStatusSqlIn");

    // 审计快照必须记录金额历史（草稿覆盖前不可丢失）
    {
        const std::string auditFn = extractFunction(service, "nlohmann::json auditSnapshot");
        contains(auditFn, "base_salary");
        contains(auditFn, "hourly_rate");
        contains(auditFn, "social_insurance_housing_fund");
        contains(auditFn, "pay_type");
        contains(auditFn, "effective_from");
        contains(auditFn, "note");
    }

    // 金额可见性：开放状态 + 当前 scope + (assignee∩propose | approve | activate)
    // note 与金额同权隐藏；list 不得 audience 旁路，active 须走 salary:read
    contains(service, "canSeeAmounts");
    contains(service, "AmountVisibilityContext");
    contains(service, "loadAmountVisibilityContext");
    contains(service, "assigneeUserId");
    contains(service, "kCompensationApprove");
    contains(service, "kSalaryProfileActivate");
    {
        // 权限/scope 一次性加载进只读上下文（list 禁止每行再查）
        const std::string loadFn =
            extractFunction(service, "AmountVisibilityContext loadAmountVisibilityContext");
        contains(loadFn, "loadEffectiveOrgScope");
        contains(loadFn, "kCompensationPropose");
        contains(loadFn, "kCompensationApprove");
        contains(loadFn, "kSalaryProfileActivate");
    }
    {
        // 纯内存 canSeeAmounts：规则未放宽
        const std::string seeFn =
            extractFunction(service, "bool canSeeAmounts(\n    const AmountVisibilityContext");
        contains(seeFn, "isOpenStatus");
        contains(seeFn, "departmentInScope");
        contains(seeFn, "assigneeUserId");
        contains(seeFn, "hasPropose");
        contains(seeFn, "hasApprove");
        contains(seeFn, "hasActivate");
        // 禁止 assignee 身份短路（权限撤销/转岗后仍见金额）
        notContains(seeFn, "if (ctx.operatorUserId == proposal.assigneeUserId)\n    {\n        return true;");
        // 必须先判开放状态，再谈权限
        const auto openPos = seeFn.find("isOpenStatus");
        const auto proposePos = seeFn.find("hasPropose");
        const auto approvePos = seeFn.find("hasApprove");
        assert(openPos != std::string::npos);
        assert(proposePos != std::string::npos);
        assert(approvePos != std::string::npos);
        assert(openPos < proposePos);
        assert(openPos < approvePos);
        // 纯内存路径不得再打 DB
        notContains(seeFn, "loadEffectiveOrgScope");
        notContains(seeFn, "hasPermission");
        notContains(seeFn, "userHasPermission");
        notContains(seeFn, "dbManager");
    }
    {
        const std::string toJsonFn =
            extractFunction(service, "nlohmann::json proposalToJson");
        // note 仅在 includeAmounts 分支写入
        contains(toJsonFn, "includeAmounts");
        contains(toJsonFn, "item[\"note\"]");
        // 非金额路径不得无条件塞 note
        const auto notePos = toJsonFn.find("item[\"note\"]");
        assert(notePos != std::string::npos);
        const auto amountsPos = toJsonFn.find("if (includeAmounts)");
        assert(amountsPos != std::string::npos);
        assert(amountsPos < notePos);
    }
    {
        // list 边界：先物化再纯内存可见性；禁止 boss/finance audience 无条件 includeAmounts
        const std::string listFn = extractFunction(service, "ListResult listProposals");
        contains(listFn, "loadAmountVisibilityContext");
        contains(listFn, "std::vector<MaterializedListRow> materialisedRows");
        contains(listFn, "for (const auto &entry : materialisedRows)");
        contains(listFn, "canSeeAmounts(amountVisibility");
        notContains(listFn, "audience == \"boss\" || audience == \"finance\"");
        notContains(listFn, "audience == \"boss\" ||\n                audience == \"finance\"");
        // includeAmounts 必须经 canSeeAmounts，不得字面 true 旁路
        const auto includePos = listFn.find("includeAmounts");
        assert(includePos != std::string::npos);
        const auto seeCallPos = listFn.find("canSeeAmounts(", includePos);
        assert(seeCallPos != std::string::npos);
        notContains(listFn, "includeAmounts = true");
        notContains(listFn, "proposalToJson(p, true");
        notContains(listFn, "proposalToJson(entry.p, true");
        // 禁止 list 使用会再查库的 canSeeAmounts(dbManager, ...) 重载
        notContains(listFn, "canSeeAmounts(dbManager");

        // fetch 循环体（物化阶段）不得调用 canSeeAmounts 或其它 DB 查询
        const auto fetchPos = listFn.find("for (mysqlx::Row row = rows.fetchOne()");
        assert(fetchPos != std::string::npos);
        const auto fetchBrace = listFn.find('{', fetchPos);
        assert(fetchBrace != std::string::npos);
        int depth = 0;
        std::size_t fetchEnd = fetchBrace;
        for (std::size_t i = fetchBrace; i < listFn.size(); ++i)
        {
            if (listFn[i] == '{')
            {
                ++depth;
            }
            else if (listFn[i] == '}')
            {
                --depth;
                if (depth == 0)
                {
                    fetchEnd = i;
                    break;
                }
            }
        }
        const std::string fetchBody = listFn.substr(fetchBrace, fetchEnd - fetchBrace + 1);
        notContains(fetchBody, "canSeeAmounts");
        notContains(fetchBody, "hasPermission");
        notContains(fetchBody, "loadEffectiveOrgScope");
        notContains(fetchBody, "userHasPermission");
        notContains(fetchBody, "session->sql");
        // 物化完成后才做金额可见性
        const auto matLoopPos = listFn.find("for (const auto &entry : materialisedRows)");
        assert(matLoopPos != std::string::npos);
        assert(fetchEnd < matLoopPos);
        assert(matLoopPos < seeCallPos);
    }

    // phase 前置 + 生效日边界
    contains(service, "phasePrerequisiteOk");
    contains(service, "hasEffectiveAssignment");
    contains(service, "validateEffectiveDateBoundary");
    contains(service, "effectiveAssignmentDate");
    contains(service, "onboard");
    contains(service, "regularize");

    // update/submit：锁提案后强制组织范围（fail closed 404）
    {
        const std::string updateFn = extractFunction(service, "OpResult updateProposal");
        const std::string submitFn = extractFunction(service, "OpResult submitProposal");
        contains(updateFn, "loadEffectiveOrgScope");
        contains(updateFn, "departmentInScope");
        contains(updateFn, "NOT_ASSIGNEE");
        contains(updateFn, "STALE_VERSION");
        contains(submitFn, "loadEffectiveOrgScope");
        contains(submitFn, "departmentInScope");
        contains(submitFn, "NOT_ASSIGNEE");
        contains(submitFn, "STALE_VERSION");
        // 范围外统一 404，避免枚举越权 ID
        contains(updateFn, "return fail(404, \"提案不存在\", \"NOT_FOUND\")");
        contains(submitFn, "return fail(404, \"提案不存在\", \"NOT_FOUND\")");
    }

    // 批准与激活路径均调用生效日边界校验
    {
        const std::string decideFn = extractFunction(service, "OpResult decideProposal");
        const std::string confirmFn = extractFunction(service, "OpResult confirmActivation");
        contains(decideFn, "validateEffectiveDateBoundary");
        contains(decideFn, "EFFECTIVE_DATE_CONFLICT");
        contains(confirmFn, "validateEffectiveDateBoundary");
        contains(confirmFn, "EFFECTIVE_DATE_CONFLICT");
        contains(confirmFn, "prepareSalaryProfileActivation");
        // 禁止未消费的 bare SELECT ... FOR UPDATE().execute()
        notContains(confirmFn, "SELECT id FROM salaryProfile WHERE user_id = ? FOR UPDATE");
        // 必须物化多行结果再继续
        contains(service, "prepareSalaryProfileActivation");
        const std::string prepFn =
            extractFunction(service, "bool prepareSalaryProfileActivation");
        contains(prepFn, "fetchOne");
        contains(prepFn, "FOR UPDATE");
        contains(prepFn, "getAffectedItemsCount() != 1");
        contains(prepFn, "DATE_SUB(?, INTERVAL 1 DAY)");
        // MySQL X：先完整消费结果集，再第二轮评估冲突
        contains(prepFn, "std::vector<ProfileRow> rows");
        contains(prepFn, "for (const auto &p : rows)");
        const std::string boundaryFn =
            extractFunction(service, "bool validateEffectiveDateBoundary");
        contains(boundaryFn, "std::vector<HistRow> histRows");
        contains(boundaryFn, "for (const auto &h : histRows)");
    }

    // 列表展示字段：userName / 快照 org 名 / 当前职位名；范围仍用快照 department
    {
        const std::string listFn = extractFunction(service, "ListResult listProposals");
        contains(listFn, "userName");
        contains(listFn, "branchName");
        contains(listFn, "departmentName");
        contains(listFn, "positionName");
        contains(listFn, "LEFT JOIN branches b ON b.id = cp.branch_id");
        contains(listFn, "LEFT JOIN departments d ON d.id = cp.department_id");
        contains(listFn, "LEFT JOIN positions pos ON pos.id = u.position_id");
        // 范围决策必须继续使用提案快照 department_id，而非展示职位
        contains(listFn, "cp.department_id IN (");
        // scope 经一次性可见性上下文加载，禁止在 list SQL 后再每行 loadEffectiveOrgScope
        contains(listFn, "loadAmountVisibilityContext");
        contains(listFn, "amountVisibility.scope");
    }

    // 固定锁顺序注释/实现
    contains(service, "proposal → employment");
    contains(service, "proposal → employment → salaryProfile");

    // 财务 return 不改 employment / 不建 profile
    const std::string returnFn =
        extractFunction(service, "OpResult returnActivation");
    notContains(returnFn, "INSERT INTO salaryProfile");
    contains(returnFn, "status = 'returned'");
    contains(returnFn, "management_approved");

    // proposed_by 固定操作人
    contains(service, "proposed_by");
    contains(service, "request.operatorUserId");

    return 0;
}
