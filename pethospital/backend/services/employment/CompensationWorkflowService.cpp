#include "CompensationWorkflowService.h"

#include "../rbac/RbacService.h"
#include "../../utils/Utils.h"
#include "../../utils/permissions/Permissions.h"

#include <algorithm>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

namespace CompensationWorkflowService
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

OpResult fail(int httpStatus, const std::string &message, const std::string &code = "")
{
    return failResult<OpResult>(httpStatus, message, code);
}

ListResult failList(int httpStatus, const std::string &message, const std::string &code = "")
{
    return failResult<ListResult>(httpStatus, message, code);
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

std::string todayDateString()
{
    return formatDateOnly(boost::posix_time::second_clock::local_time());
}

bool isValidDate(const std::string &value)
{
    if (value.size() != 10 || value[4] != '-' || value[7] != '-')
    {
        return false;
    }
    try
    {
        return boost::gregorian::to_iso_extended_string(
                   boost::gregorian::from_simple_string(value)) == value;
    }
    catch (...)
    {
        return false;
    }
}

bool isValidPhase(const std::string &phase)
{
    return phase == "probation" || phase == "regular" || phase == "adjustment";
}

bool isValidPayType(const std::string &payType)
{
    return payType == "monthly" || payType == "hourly";
}

bool isOpenStatus(const std::string &status)
{
    // v6 open-case：含 finance_confirmed，防止该状态落库后绕过同阶段唯一开放约束。
    return status == "draft" || status == "submitted" || status == "management_approved" ||
           status == "returned" || status == "finance_confirmed";
}

// SQL IN 列表，与 isOpenStatus 保持一致。
const char *kOpenStatusSqlIn =
    "'draft','submitted','management_approved','returned','finance_confirmed'";

bool isValidListStatus(const std::string &status)
{
    return status.empty() || status == "draft" || status == "submitted" ||
           status == "management_approved" || status == "returned" ||
           status == "finance_confirmed" || status == "active" || status == "cancelled";
}

bool hasPermission(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId,
    const char *key)
{
    return RbacService::userHasPermission(dbManager, userId, key);
}

bool userHasPermissionInSession(
    mysqlx::Session &session,
    int userId,
    const char *permissionKey)
{
    if (userId <= 0 || permissionKey == nullptr)
    {
        return false;
    }
    mysqlx::Row pos = session
                          .sql("SELECT 1 FROM users u "
                               "JOIN position_permissions pp ON pp.position_id = u.position_id "
                               "WHERE u.id = ? AND u.is_deleted = 0 AND pp.permission_key = ? "
                               "LIMIT 1")
                          .bind(userId, permissionKey)
                          .execute()
                          .fetchOne();
    if (pos)
    {
        return true;
    }
    mysqlx::Row up = session
                         .sql("SELECT 1 FROM user_permissions "
                              "WHERE user_id = ? AND permission_key = ? LIMIT 1")
                         .bind(userId, permissionKey)
                         .execute()
                         .fetchOne();
    return static_cast<bool>(up);
}

bool nonNegativeFinite(double value)
{
    return std::isfinite(value) && value >= 0.0;
}

struct PayFields
{
    std::string payType;
    std::optional<double> baseSalary;
    std::optional<double> hourlyRate;
    double social = 0.0;
};

bool normalizePayFields(
    const std::string &payType,
    const std::optional<double> &baseSalary,
    const std::optional<double> &hourlyRate,
    double social,
    PayFields &out,
    std::string &error)
{
    if (!isValidPayType(payType))
    {
        error = "payType 取值不合法";
        return false;
    }
    if (!nonNegativeFinite(social))
    {
        error = "socialInsuranceHousingFund 必须非负";
        return false;
    }
    out.payType = payType;
    out.social = social;
    if (payType == "monthly")
    {
        if (!baseSalary.has_value() || !nonNegativeFinite(*baseSalary))
        {
            error = "月薪必须提供非负 baseSalary";
            return false;
        }
        if (hourlyRate.has_value())
        {
            error = "月薪不得同时提供 hourlyRate";
            return false;
        }
        out.baseSalary = *baseSalary;
        out.hourlyRate = std::nullopt;
        return true;
    }
    if (!hourlyRate.has_value() || !nonNegativeFinite(*hourlyRate))
    {
        error = "时薪必须提供非负 hourlyRate";
        return false;
    }
    if (baseSalary.has_value())
    {
        error = "时薪不得同时提供 baseSalary";
        return false;
    }
    out.hourlyRate = *hourlyRate;
    out.baseSalary = std::nullopt;
    return true;
}

void insertWorkflowAudit(
    mysqlx::Session &session,
    long long resourceId,
    const std::string &actionName,
    int operatorId,
    int branchId,
    int departmentId,
    const nlohmann::json &before,
    const nlohmann::json &after,
    const std::string &reason)
{
    // 审计快照含金额字段，供职责分离追溯；读路径须另做薪资敏感权限 + 范围校验。
    std::string sql =
        "INSERT INTO employment_workflow_audit "
        "(resource_type, resource_id, action, operator_id, branch_id, department_id, "
        "before_snapshot, after_snapshot, reason) VALUES ('compensation', ?, ?, ";
    sql += operatorId > 0 ? "?" : "NULL";
    sql += ", ";
    sql += branchId > 0 ? "?" : "NULL";
    sql += ", ";
    sql += departmentId > 0 ? "?" : "NULL";
    sql += ", ?, ?, ?)";

    auto stmt = session.sql(sql).bind(resourceId, actionName);
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

struct LockedProposal
{
    long long id = 0;
    long long employmentId = 0;
    int branchId = 0;
    int departmentId = 0;
    std::string phase;
    std::string payType;
    std::optional<double> baseSalary;
    std::optional<double> hourlyRate;
    double social = 0.0;
    std::string effectiveFrom;
    std::string status;
    int assigneeUserId = 0;
    int proposedBy = 0;
    int submittedBy = 0;
    int approvedBy = 0;
    int financeConfirmedBy = 0;
    int salaryProfileId = 0;
    std::string note;
    int expectedEmploymentRowVersion = 0;
    bool hasExpectedEmploymentRowVersion = false;
    int rowVersion = 1;
};

std::optional<LockedProposal> lockProposal(mysqlx::Session &session, long long proposalId)
{
    if (proposalId <= 0)
    {
        return std::nullopt;
    }
    mysqlx::Row row = session
                          .sql("SELECT id, employment_id, branch_id, department_id, phase, pay_type, "
                               "base_salary, hourly_rate, social_insurance_housing_fund, "
                               "CAST(effective_from AS CHAR), status, assignee_user_id, proposed_by, "
                               "COALESCE(submitted_by, 0), COALESCE(approved_by, 0), "
                               "COALESCE(finance_confirmed_by, 0), COALESCE(salary_profile_id, 0), "
                               "COALESCE(note, ''), expected_employment_row_version, row_version "
                               "FROM compensation_proposal WHERE id = ? LIMIT 1 FOR UPDATE")
                          .bind(proposalId)
                          .execute()
                          .fetchOne();
    if (!row)
    {
        return std::nullopt;
    }
    LockedProposal p;
    p.id = row[0].get<int64_t>();
    p.employmentId = row[1].get<int64_t>();
    p.branchId = row[2].isNull() ? 0 : row[2].get<int>();
    p.departmentId = row[3].isNull() ? 0 : row[3].get<int>();
    p.phase = row[4].isNull() ? "" : row[4].get<std::string>();
    p.payType = row[5].isNull() ? "" : row[5].get<std::string>();
    if (!row[6].isNull())
    {
        p.baseSalary = row[6].get<double>();
    }
    if (!row[7].isNull())
    {
        p.hourlyRate = row[7].get<double>();
    }
    p.social = row[8].isNull() ? 0.0 : row[8].get<double>();
    p.effectiveFrom = row[9].isNull() ? "" : row[9].get<std::string>();
    p.status = row[10].isNull() ? "" : row[10].get<std::string>();
    p.assigneeUserId = row[11].isNull() ? 0 : row[11].get<int>();
    p.proposedBy = row[12].isNull() ? 0 : row[12].get<int>();
    p.submittedBy = row[13].isNull() ? 0 : row[13].get<int>();
    p.approvedBy = row[14].isNull() ? 0 : row[14].get<int>();
    p.financeConfirmedBy = row[15].isNull() ? 0 : row[15].get<int>();
    p.salaryProfileId = row[16].isNull() ? 0 : row[16].get<int>();
    p.note = row[17].isNull() ? "" : row[17].get<std::string>();
    if (!row[18].isNull())
    {
        p.expectedEmploymentRowVersion = row[18].get<int>();
        p.hasExpectedEmploymentRowVersion = true;
    }
    p.rowVersion = row[19].isNull() ? 1 : row[19].get<int>();
    return p;
}

struct LockedEmployment
{
    long long id = 0;
    int userId = 0;
    std::string status;
    int rowVersion = 1;
    int probationWaived = 0;
    int positionId = 0;
    int branchId = 0;
    int departmentId = 0;
    int isDeleted = 0;
};

std::optional<LockedEmployment> lockEmploymentById(mysqlx::Session &session, long long employmentId)
{
    mysqlx::Row row = session
                          .sql("SELECT e.id, e.user_id, e.status, e.row_version, "
                               "COALESCE(e.probation_waived, 0), COALESCE(u.position_id, 0), "
                               "COALESCE(d.branch_id, 0), COALESCE(p.department_id, 0), "
                               "COALESCE(u.is_deleted, 0) "
                               "FROM employment e "
                               "JOIN users u ON u.id = e.user_id "
                               "LEFT JOIN positions p ON p.id = u.position_id "
                               "LEFT JOIN departments d ON d.id = p.department_id "
                               "WHERE e.id = ? LIMIT 1 FOR UPDATE")
                          .bind(employmentId)
                          .execute()
                          .fetchOne();
    if (!row)
    {
        return std::nullopt;
    }
    LockedEmployment e;
    e.id = row[0].get<int64_t>();
    e.userId = row[1].get<int>();
    e.status = row[2].isNull() ? "" : row[2].get<std::string>();
    e.rowVersion = row[3].isNull() ? 1 : row[3].get<int>();
    e.probationWaived = row[4].isNull() ? 0 : row[4].get<int>();
    e.positionId = row[5].isNull() ? 0 : row[5].get<int>();
    e.branchId = row[6].isNull() ? 0 : row[6].get<int>();
    e.departmentId = row[7].isNull() ? 0 : row[7].get<int>();
    e.isDeleted = row[8].isNull() ? 0 : row[8].get<int>();
    return e;
}

bool hasEffectiveAssignment(
    mysqlx::Session &session,
    long long employmentId,
    const std::string &action)
{
    mysqlx::Row row = session
                          .sql("SELECT 1 FROM employment_assignment "
                               "WHERE employment_id = ? AND action = ? AND status = 'effective' "
                               "LIMIT 1")
                          .bind(employmentId, action)
                          .execute()
                          .fetchOne();
    return static_cast<bool>(row);
}

// 取该 action 下最早生效的任职决定日期（YYYY-MM-DD）。
std::optional<std::string> effectiveAssignmentDate(
    mysqlx::Session &session,
    long long employmentId,
    const std::string &action)
{
    mysqlx::Row row = session
                          .sql("SELECT CAST(effective_from AS CHAR) FROM employment_assignment "
                               "WHERE employment_id = ? AND action = ? AND status = 'effective' "
                               "ORDER BY effective_from ASC, id ASC LIMIT 1")
                          .bind(employmentId, action)
                          .execute()
                          .fetchOne();
    if (!row || row[0].isNull())
    {
        return std::nullopt;
    }
    return row[0].get<std::string>();
}

// 管理批准前与财务激活前：校验 effective_from 不早于依赖的任职生效边界。
// 草稿创建不强制（转正决定可能尚未存在）；此处 fail closed。
bool validateEffectiveDateBoundary(
    mysqlx::Session &session,
    const LockedProposal &proposal,
    const LockedEmployment &employment,
    std::string &errorCode,
    std::string &message)
{
    if (proposal.phase == "probation")
    {
        const auto boundary = effectiveAssignmentDate(session, employment.id, "onboard");
        if (!boundary.has_value() || proposal.effectiveFrom < *boundary)
        {
            errorCode = "EFFECTIVE_DATE_CONFLICT";
            message = "试用期薪酬生效日不得早于入职任职生效日";
            return false;
        }
        return true;
    }
    if (proposal.phase == "regular")
    {
        if (employment.probationWaived == 1)
        {
            const auto boundary = effectiveAssignmentDate(session, employment.id, "onboard");
            if (!boundary.has_value() || proposal.effectiveFrom < *boundary)
            {
                errorCode = "EFFECTIVE_DATE_CONFLICT";
                message = "免试用正式薪酬生效日不得早于入职任职生效日";
                return false;
            }
            return true;
        }
        const auto boundary = effectiveAssignmentDate(session, employment.id, "regularize");
        if (!boundary.has_value() || proposal.effectiveFrom < *boundary)
        {
            errorCode = "EFFECTIVE_DATE_CONFLICT";
            message = "正式薪酬生效日不得早于转正任职生效日";
            return false;
        }
        return true;
    }
    if (proposal.phase == "adjustment")
    {
        if (employment.status != "active")
        {
            errorCode = "EMPLOYMENT_STATE_CONFLICT";
            message = "调薪要求 employment.status=active";
            return false;
        }
        // 相对薪资历史：禁止未来/重叠/回写；当前 open 行允许（激活时关闭）。
        // MySQL X 兼容边界：必须先完整消费结果集，再在第二轮评估冲突（禁止 fetch 中途 return）。
        mysqlx::SqlResult hist = session
                                     .sql("SELECT CAST(effective_from AS CHAR), "
                                          "CAST(effective_to AS CHAR) "
                                          "FROM salaryProfile WHERE user_id = ? "
                                          "ORDER BY effective_from ASC, id ASC")
                                     .bind(employment.userId)
                                     .execute();
        struct HistRow
        {
            std::string from;
            std::string to;
        };
        std::vector<HistRow> histRows;
        for (mysqlx::Row row = hist.fetchOne(); row; row = hist.fetchOne())
        {
            HistRow h;
            h.from = row[0].isNull() ? "" : row[0].get<std::string>();
            h.to = row[1].isNull() ? "" : row[1].get<std::string>();
            if (!h.from.empty())
            {
                histRows.push_back(std::move(h));
            }
        }
        for (const auto &h : histRows)
        {
            if (h.from >= proposal.effectiveFrom)
            {
                errorCode = "EFFECTIVE_DATE_CONFLICT";
                message = "调薪生效日与既有薪资配置冲突（含未来区间）";
                return false;
            }
            // 已关闭区间延伸到新生效日 → 重叠；open 行 (to IS NULL) 留给激活阶段关闭
            if (!h.to.empty() && h.to >= proposal.effectiveFrom)
            {
                errorCode = "EFFECTIVE_DATE_CONFLICT";
                message = "调薪生效日与已关闭薪资历史重叠";
                return false;
            }
        }
        return true;
    }
    errorCode = "INVALID_PHASE";
    message = "phase 取值不合法";
    return false;
}

// 锁并物化用户全部 salaryProfile 行；拒绝未来/重叠区间；关闭当前 open 行时校验 affected=1。
// 返回 false 时 errorCode/message 已填充。
bool prepareSalaryProfileActivation(
    mysqlx::Session &session,
    int userId,
    const std::string &newEffectiveFrom,
    std::string &errorCode,
    std::string &message)
{
    mysqlx::SqlResult locked =
        session
            .sql("SELECT id, CAST(effective_from AS CHAR), CAST(effective_to AS CHAR) "
                 "FROM salaryProfile WHERE user_id = ? "
                 "ORDER BY effective_from ASC, id ASC FOR UPDATE")
            .bind(userId)
            .execute();

    struct ProfileRow
    {
        int id = 0;
        std::string from;
        std::string to; // empty = open-ended
    };
    // MySQL X 兼容边界：先完整消费 FOR UPDATE 结果集，再第二轮评估冲突（禁止 fetch 中途 return）。
    std::vector<ProfileRow> rows;
    for (mysqlx::Row row = locked.fetchOne(); row; row = locked.fetchOne())
    {
        ProfileRow p;
        p.id = row[0].isNull() ? 0 : row[0].get<int>();
        p.from = row[1].isNull() ? "" : row[1].get<std::string>();
        p.to = row[2].isNull() ? "" : row[2].get<std::string>();
        if (p.id <= 0 || p.from.empty())
        {
            continue;
        }
        rows.push_back(std::move(p));
    }

    int openId = 0;
    for (const auto &p : rows)
    {
        // 未来起点或同日：无论是否已关闭，均不可静默重叠
        if (p.from >= newEffectiveFrom)
        {
            errorCode = "SALARY_PROFILE_OVERLAP";
            message = "薪资配置生效区间重叠或存在未来配置";
            return false;
        }
        // 已关闭但区间延伸到新生效日：冲突
        if (!p.to.empty() && p.to >= newEffectiveFrom)
        {
            errorCode = "SALARY_PROFILE_OVERLAP";
            message = "薪资配置生效区间与已关闭历史重叠";
            return false;
        }
        if (p.to.empty())
        {
            if (openId != 0)
            {
                errorCode = "SALARY_PROFILE_OVERLAP";
                message = "存在多条未关闭薪资配置";
                return false;
            }
            openId = p.id;
        }
    }

    if (openId > 0)
    {
        auto closed =
            session
                .sql("UPDATE salaryProfile SET effective_to = DATE_SUB(?, INTERVAL 1 DAY) "
                     "WHERE id = ? AND user_id = ? AND effective_to IS NULL")
                .bind(newEffectiveFrom, openId, userId)
                .execute();
        if (closed.getAffectedItemsCount() != 1)
        {
            errorCode = "SALARY_PROFILE_OVERLAP";
            message = "关闭上一条薪资配置失败，请重试";
            return false;
        }
    }
    return true;
}

// 管理批准前：按 phase 校验任职前置。
bool phasePrerequisiteOk(
    mysqlx::Session &session,
    const std::string &phase,
    const LockedEmployment &employment,
    std::string &errorCode,
    std::string &message)
{
    if (employment.status == "separated" || employment.status == "rejected")
    {
        errorCode = "EMPLOYMENT_STATE_CONFLICT";
        message = "员工任职已结束，不能审批薪酬";
        return false;
    }
    if (phase == "probation")
    {
        if (employment.status != "onboarding" || employment.probationWaived != 0)
        {
            errorCode = "EMPLOYMENT_STATE_CONFLICT";
            message = "试用期薪酬要求 onboarding 且未免试用";
            return false;
        }
        if (!hasEffectiveAssignment(session, employment.id, "onboard"))
        {
            errorCode = "EMPLOYMENT_STATE_CONFLICT";
            message = "入职任职决定尚未生效";
            return false;
        }
        return true;
    }
    if (phase == "regular")
    {
        if (employment.probationWaived == 1)
        {
            if (employment.status != "onboarding")
            {
                errorCode = "EMPLOYMENT_STATE_CONFLICT";
                message = "免试用正式薪酬要求 onboarding 状态";
                return false;
            }
            if (!hasEffectiveAssignment(session, employment.id, "onboard"))
            {
                errorCode = "EMPLOYMENT_STATE_CONFLICT";
                message = "入职任职决定尚未生效";
                return false;
            }
            return true;
        }
        if (employment.status != "regularization_pending")
        {
            errorCode = "EMPLOYMENT_STATE_CONFLICT";
            message = "普通正式薪酬要求 regularization_pending";
            return false;
        }
        if (!hasEffectiveAssignment(session, employment.id, "regularize"))
        {
            errorCode = "EMPLOYMENT_STATE_CONFLICT";
            message = "转正任职决定尚未生效";
            return false;
        }
        return true;
    }
    if (phase == "adjustment")
    {
        if (employment.status != "active")
        {
            errorCode = "EMPLOYMENT_STATE_CONFLICT";
            message = "调薪要求 employment.status=active";
            return false;
        }
        return true;
    }
    errorCode = "INVALID_PHASE";
    message = "phase 取值不合法";
    return false;
}

// 列表/读路径金额可见性（v6）：仅开放案件；须当前 org scope 覆盖提案部门快照；
// 操作者金额可见性上下文：list 路径一次性加载后纯内存判断，避免 MySQL X 结果集未消费时再发 SQL。
struct AmountVisibilityContext
{
    int operatorUserId = 0;
    RbacService::EffectiveOrgScope scope;
    bool hasPropose = false;
    bool hasApprove = false;
    bool hasActivate = false;
};

AmountVisibilityContext loadAmountVisibilityContext(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId)
{
    AmountVisibilityContext ctx;
    ctx.operatorUserId = operatorUserId;
    if (operatorUserId <= 0 || !dbManager)
    {
        return ctx;
    }
    ctx.scope = RbacService::loadEffectiveOrgScope(dbManager, operatorUserId);
    ctx.hasPropose = hasPermission(dbManager, operatorUserId, Permissions::kCompensationPropose);
    ctx.hasApprove = hasPermission(dbManager, operatorUserId, Permissions::kCompensationApprove);
    ctx.hasActivate =
        hasPermission(dbManager, operatorUserId, Permissions::kSalaryProfileActivate);
    return ctx;
}

// 纯内存金额可见性：开放状态 + 当前 scope + (assignee∩propose | approve | activate)。
// 激活后权威金额走 salaryProfile + salary:read，不得经 list 用 audience 旁路回读 active 金额。
// 写路径成功响应可直接回显刚操作数据，不经本 helper。
bool canSeeAmounts(
    const AmountVisibilityContext &ctx,
    const LockedProposal &proposal)
{
    if (ctx.operatorUserId <= 0)
    {
        return false;
    }
    // active/cancelled 等非开放状态：list 不得暴露提案金额
    if (!isOpenStatus(proposal.status))
    {
        return false;
    }
    if (!departmentInScope(ctx.scope, proposal.departmentId))
    {
        return false;
    }
    // 受理人必须仍持 propose；权限撤销或调岗后不得因历史 assignee 身份继续读金额
    if (ctx.operatorUserId == proposal.assigneeUserId && ctx.hasPropose)
    {
        return true;
    }
    if (ctx.hasApprove)
    {
        return true;
    }
    if (ctx.hasActivate)
    {
        return true;
    }
    return false;
}

// 单条写路径响应用：无未消费结果集时加载上下文再判断。
bool canSeeAmounts(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId,
    const LockedProposal &proposal)
{
    return canSeeAmounts(loadAmountVisibilityContext(dbManager, operatorUserId), proposal);
}

nlohmann::json proposalToJson(
    const LockedProposal &p,
    bool includeAmounts,
    int employmentUserId = 0,
    const std::string &employmentStatus = "")
{
    nlohmann::json item = {
        {"id", p.id},
        {"employmentId", p.employmentId},
        {"branchId", p.branchId},
        {"departmentId", p.departmentId},
        {"phase", p.phase},
        {"status", p.status},
        {"assigneeUserId", p.assigneeUserId},
        {"proposedBy", p.proposedBy},
        {"submittedBy", p.submittedBy > 0 ? nlohmann::json(p.submittedBy) : nlohmann::json(nullptr)},
        {"approvedBy", p.approvedBy > 0 ? nlohmann::json(p.approvedBy) : nlohmann::json(nullptr)},
        {"financeConfirmedBy",
         p.financeConfirmedBy > 0 ? nlohmann::json(p.financeConfirmedBy) : nlohmann::json(nullptr)},
        {"salaryProfileId",
         p.salaryProfileId > 0 ? nlohmann::json(p.salaryProfileId) : nlohmann::json(nullptr)},
        {"effectiveFrom", p.effectiveFrom},
        {"expectedEmploymentRowVersion",
         p.hasExpectedEmploymentRowVersion ? nlohmann::json(p.expectedEmploymentRowVersion)
                                           : nlohmann::json(nullptr)},
        {"rowVersion", p.rowVersion},
    };
    if (employmentUserId > 0)
    {
        item["userId"] = employmentUserId;
    }
    if (!employmentStatus.empty())
    {
        item["employmentStatus"] = employmentStatus;
    }
    if (includeAmounts)
    {
        // note 与金额同级敏感：无金额可见性时一并省略，防协商备注外泄。
        item["payType"] = p.payType;
        item["baseSalary"] =
            p.baseSalary.has_value() ? nlohmann::json(*p.baseSalary) : nlohmann::json(nullptr);
        item["hourlyRate"] =
            p.hourlyRate.has_value() ? nlohmann::json(*p.hourlyRate) : nlohmann::json(nullptr);
        item["socialInsuranceHousingFund"] = p.social;
        item["note"] = p.note;
    }
    return item;
}

nlohmann::json auditSnapshot(const LockedProposal &p, const LockedEmployment *emp = nullptr)
{
    // 含金额与 note：草稿改写需留下不可变金额历史；读接口须另做薪资敏感权限。
    nlohmann::json snap = {
        {"proposal_id", p.id},
        {"employment_id", p.employmentId},
        {"phase", p.phase},
        {"status", p.status},
        {"assignee_user_id", p.assigneeUserId},
        {"proposed_by", p.proposedBy},
        {"approved_by", p.approvedBy},
        {"finance_confirmed_by", p.financeConfirmedBy},
        {"org_branch_id", p.branchId},
        {"org_department_id", p.departmentId},
        {"row_version", p.rowVersion},
        {"expected_employment_row_version",
         p.hasExpectedEmploymentRowVersion ? nlohmann::json(p.expectedEmploymentRowVersion)
                                           : nlohmann::json(nullptr)},
        {"salary_profile_id",
         p.salaryProfileId > 0 ? nlohmann::json(p.salaryProfileId) : nlohmann::json(nullptr)},
        {"effective_from", p.effectiveFrom},
        {"pay_type", p.payType},
        {"base_salary",
         p.baseSalary.has_value() ? nlohmann::json(*p.baseSalary) : nlohmann::json(nullptr)},
        {"hourly_rate",
         p.hourlyRate.has_value() ? nlohmann::json(*p.hourlyRate) : nlohmann::json(nullptr)},
        {"social_insurance_housing_fund", p.social},
        {"note", p.note},
    };
    if (emp != nullptr)
    {
        snap["employment_status"] = emp->status;
        snap["employment_row_version"] = emp->rowVersion;
        snap["probation_waived"] = emp->probationWaived;
        snap["user_id"] = emp->userId;
    }
    return snap;
}

// 校验用户持 compensation:propose 且组织范围覆盖 department。
bool userCanProposeOnDepartment(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    mysqlx::Session &session,
    int userId,
    int departmentId)
{
    if (!userHasPermissionInSession(session, userId, Permissions::kCompensationPropose) &&
        !hasPermission(dbManager, userId, Permissions::kCompensationPropose))
    {
        return false;
    }
    const auto scope = RbacService::loadEffectiveOrgScope(dbManager, userId);
    return departmentInScope(scope, departmentId);
}

} // namespace

ListResult listProposals(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const ListQuery &query)
{
    if (!dbManager || !dbManager->getSession())
    {
        return failList(500, "数据库不可用");
    }
    if (query.operatorUserId <= 0)
    {
        return failList(400, "用户无效", "INVALID_USER");
    }
    if (!isValidListStatus(query.status))
    {
        return failList(400, "status 取值不合法", "INVALID_STATUS");
    }
    if (!query.phase.empty() && !isValidPhase(query.phase))
    {
        return failList(400, "phase 取值不合法", "INVALID_PHASE");
    }

    const std::string audience = query.audience.empty() ? "personnel" : query.audience;
    if (audience == "personnel")
    {
        if (!hasPermission(dbManager, query.operatorUserId, Permissions::kCompensationPropose) &&
            !hasPermission(dbManager, query.operatorUserId, Permissions::kCompensationReassignCase))
        {
            return failList(403, "缺少薪酬拟案权限", "PROPOSE_DENIED");
        }
    }
    else if (audience == "boss")
    {
        if (!hasPermission(dbManager, query.operatorUserId, Permissions::kCompensationApprove))
        {
            return failList(403, "缺少薪酬审批权限", "APPROVE_DENIED");
        }
    }
    else if (audience == "finance")
    {
        if (!hasPermission(dbManager, query.operatorUserId, Permissions::kSalaryProfileActivate))
        {
            return failList(403, "缺少薪资配置激活权限", "ACTIVATE_DENIED");
        }
    }
    else
    {
        return failList(400, "audience 取值不合法", "INVALID_AUDIENCE");
    }

    const int page = std::max(1, query.page);
    const int pageSize = std::min(100, std::max(1, query.pageSize));
    const long long offset = static_cast<long long>(page - 1) * pageSize;
    // 列表范围与金额可见性共用一次 scope/权限加载；禁止在 fetch 循环内再查库。
    const AmountVisibilityContext amountVisibility =
        loadAmountVisibilityContext(dbManager, query.operatorUserId);
    const auto &scope = amountVisibility.scope;

    ListResult result;
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
            where += " AND cp.status = ? ";
        }
        else if (audience == "boss")
        {
            where += " AND cp.status = 'submitted' ";
        }
        else if (audience == "finance")
        {
            where += " AND cp.status = 'management_approved' ";
        }
        if (!query.phase.empty())
        {
            where += " AND cp.phase = ? ";
        }
        if (!scope.unrestricted)
        {
            if (scope.departmentIds.empty())
            {
                result.total = 0;
                return result;
            }
            where += " AND cp.department_id IN (" + joinDepartmentIds(scope.departmentIds) + ") ";
        }

        {
            std::string countSql = "SELECT COUNT(*) FROM compensation_proposal cp " + where;
            auto stmt = session->sql(countSql);
            if (!query.status.empty())
            {
                stmt.bind(query.status);
            }
            if (!query.phase.empty())
            {
                stmt.bind(query.phase);
            }
            mysqlx::Row countRow = stmt.execute().fetchOne();
            result.total = countRow && !countRow[0].isNull() ? countRow[0].get<int>() : 0;
        }

        // 列表展示名仅用于 UI；授权/范围仍只用提案快照 branch_id/department_id，不用当前职位。
        std::string listSql =
            "SELECT cp.id, cp.employment_id, cp.branch_id, cp.department_id, cp.phase, cp.pay_type, "
            "cp.base_salary, cp.hourly_rate, cp.social_insurance_housing_fund, "
            "CAST(cp.effective_from AS CHAR), cp.status, cp.assignee_user_id, cp.proposed_by, "
            "COALESCE(cp.submitted_by, 0), COALESCE(cp.approved_by, 0), "
            "COALESCE(cp.finance_confirmed_by, 0), COALESCE(cp.salary_profile_id, 0), "
            "COALESCE(cp.note, ''), cp.expected_employment_row_version, cp.row_version, "
            "e.user_id, e.status, "
            "COALESCE(u.name, ''), COALESCE(b.name, ''), COALESCE(d.name, ''), COALESCE(pos.name, '') "
            "FROM compensation_proposal cp "
            "JOIN employment e ON e.id = cp.employment_id "
            "LEFT JOIN users u ON u.id = e.user_id "
            "LEFT JOIN branches b ON b.id = cp.branch_id "
            "LEFT JOIN departments d ON d.id = cp.department_id "
            "LEFT JOIN positions pos ON pos.id = u.position_id " +
            where +
            " ORDER BY cp.created_at DESC, cp.id DESC "
            "LIMIT ? OFFSET ?";
        auto stmt = session->sql(listSql);
        if (!query.status.empty())
        {
            stmt.bind(query.status);
        }
        if (!query.phase.empty())
        {
            stmt.bind(query.phase);
        }
        stmt.bind(pageSize, offset);
        mysqlx::SqlResult rows = stmt.execute();

        // MySQL X：先完整物化结果集，再做金额可见性；禁止 fetch 循环内 canSeeAmounts / 其它 SQL。
        struct MaterializedListRow
        {
            LockedProposal p;
            int userId = 0;
            std::string empStatus;
            std::string userName;
            std::string branchName;
            std::string departmentName;
            std::string positionName;
        };
        std::vector<MaterializedListRow> materialisedRows;
        for (mysqlx::Row row = rows.fetchOne(); row; row = rows.fetchOne())
        {
            MaterializedListRow entry;
            entry.p.id = row[0].get<int64_t>();
            entry.p.employmentId = row[1].get<int64_t>();
            entry.p.branchId = row[2].isNull() ? 0 : row[2].get<int>();
            entry.p.departmentId = row[3].isNull() ? 0 : row[3].get<int>();
            entry.p.phase = row[4].isNull() ? "" : row[4].get<std::string>();
            entry.p.payType = row[5].isNull() ? "" : row[5].get<std::string>();
            if (!row[6].isNull())
            {
                entry.p.baseSalary = row[6].get<double>();
            }
            if (!row[7].isNull())
            {
                entry.p.hourlyRate = row[7].get<double>();
            }
            entry.p.social = row[8].isNull() ? 0.0 : row[8].get<double>();
            entry.p.effectiveFrom = row[9].isNull() ? "" : row[9].get<std::string>();
            entry.p.status = row[10].isNull() ? "" : row[10].get<std::string>();
            entry.p.assigneeUserId = row[11].isNull() ? 0 : row[11].get<int>();
            entry.p.proposedBy = row[12].isNull() ? 0 : row[12].get<int>();
            entry.p.submittedBy = row[13].isNull() ? 0 : row[13].get<int>();
            entry.p.approvedBy = row[14].isNull() ? 0 : row[14].get<int>();
            entry.p.financeConfirmedBy = row[15].isNull() ? 0 : row[15].get<int>();
            entry.p.salaryProfileId = row[16].isNull() ? 0 : row[16].get<int>();
            entry.p.note = row[17].isNull() ? "" : row[17].get<std::string>();
            if (!row[18].isNull())
            {
                entry.p.expectedEmploymentRowVersion = row[18].get<int>();
                entry.p.hasExpectedEmploymentRowVersion = true;
            }
            entry.p.rowVersion = row[19].isNull() ? 1 : row[19].get<int>();
            entry.userId = row[20].isNull() ? 0 : row[20].get<int>();
            entry.empStatus = row[21].isNull() ? "" : row[21].get<std::string>();
            entry.userName = row[22].isNull() ? "" : row[22].get<std::string>();
            entry.branchName = row[23].isNull() ? "" : row[23].get<std::string>();
            entry.departmentName = row[24].isNull() ? "" : row[24].get<std::string>();
            entry.positionName = row[25].isNull() ? "" : row[25].get<std::string>();
            materialisedRows.push_back(std::move(entry));
        }

        // 结果集已完全消费；人事/老板/财务列表共用纯内存 canSeeAmounts，禁止 audience 旁路金额。
        for (const auto &entry : materialisedRows)
        {
            const bool includeAmounts = canSeeAmounts(amountVisibility, entry.p);
            nlohmann::json item =
                proposalToJson(entry.p, includeAmounts, entry.userId, entry.empStatus);
            item["userName"] = entry.userName;
            item["branchName"] = entry.branchName;
            item["departmentName"] = entry.departmentName;
            item["positionName"] = entry.positionName;
            result.items.push_back(std::move(item));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "CompensationWorkflowService::listProposals failed: " << e.what() << std::endl;
        return failList(500, std::string("查询失败: ") + e.what());
    }
    return result;
}

OpResult createProposal(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const CreateRequest &request)
{
    if (!dbManager || !dbManager->getSession())
    {
        return fail(500, "数据库不可用");
    }
    if (request.operatorUserId <= 0 || request.employmentId <= 0)
    {
        return fail(400, "参数无效", "INVALID_REQUEST");
    }
    if (!hasPermission(dbManager, request.operatorUserId, Permissions::kCompensationPropose))
    {
        return fail(403, "缺少薪酬拟案权限", "PROPOSE_DENIED");
    }
    if (!isValidPhase(request.phase))
    {
        return fail(400, "phase 取值不合法", "INVALID_PHASE");
    }
    if (!isValidDate(request.effectiveFrom))
    {
        return fail(400, "effectiveFrom 格式无效", "INVALID_DATE");
    }
    PayFields pay;
    std::string payError;
    if (!normalizePayFields(
            request.payType, request.baseSalary, request.hourlyRate,
            request.socialInsuranceHousingFund, pay, payError))
    {
        return fail(400, payError, "INVALID_PAY");
    }

    const int assigneeId =
        request.assigneeUserId > 0 ? request.assigneeUserId : request.operatorUserId;
    const auto operatorScope =
        RbacService::loadEffectiveOrgScope(dbManager, request.operatorUserId);

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        auto employment = lockEmploymentById(*session, request.employmentId);
        if (!employment.has_value() || employment->isDeleted != 0)
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "任职不存在", "NOT_FOUND");
        }
        if (employment->branchId <= 0 || employment->departmentId <= 0)
        {
            rollbackTransactionQuietly(*session);
            return fail(400, "无法解析组织范围快照", "SCOPE_SNAPSHOT_MISSING");
        }
        if (!departmentInScope(operatorScope, employment->departmentId))
        {
            rollbackTransactionQuietly(*session);
            return fail(403, "目标任职不在组织范围内", "OUT_OF_SCOPE");
        }
        if (!userCanProposeOnDepartment(
                dbManager, *session, assigneeId, employment->departmentId))
        {
            rollbackTransactionQuietly(*session);
            return fail(400, "初始受理人必须持 compensation:propose 且范围覆盖案件",
                        "ASSIGNEE_INVALID");
        }

        // 同阶段开放案件并发（含 finance_confirmed，与 open_slot / isOpenStatus 一致）
        mysqlx::Row openRow =
            session
                ->sql(std::string("SELECT id FROM compensation_proposal "
                                  "WHERE employment_id = ? AND phase = ? "
                                  "AND status IN (") +
                      kOpenStatusSqlIn +
                      ") LIMIT 1 FOR UPDATE")
                .bind(request.employmentId, request.phase)
                .execute()
                .fetchOne();
        if (openRow)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "同阶段已有进行中的薪酬案件", "OPEN_PROPOSAL_EXISTS");
        }

        auto ins = session
                       ->sql("INSERT INTO compensation_proposal "
                             "(employment_id, branch_id, department_id, phase, pay_type, "
                             "base_salary, hourly_rate, social_insurance_housing_fund, "
                             "effective_from, status, assignee_user_id, proposed_by, note) "
                             "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 'draft', ?, ?, ?)")
                       .bind(request.employmentId, employment->branchId, employment->departmentId,
                             request.phase, pay.payType,
                             pay.baseSalary.has_value() ? mysqlx::Value(*pay.baseSalary)
                                                        : mysqlx::Value(),
                             pay.hourlyRate.has_value() ? mysqlx::Value(*pay.hourlyRate)
                                                        : mysqlx::Value(),
                             pay.social, request.effectiveFrom, assigneeId,
                             request.operatorUserId, request.note.substr(0, 1000))
                       .execute();
        const long long proposalId = static_cast<long long>(ins.getAutoIncrementValue());

        LockedProposal created;
        created.id = proposalId;
        created.employmentId = request.employmentId;
        created.branchId = employment->branchId;
        created.departmentId = employment->departmentId;
        created.phase = request.phase;
        created.payType = pay.payType;
        created.baseSalary = pay.baseSalary;
        created.hourlyRate = pay.hourlyRate;
        created.social = pay.social;
        created.effectiveFrom = request.effectiveFrom;
        created.status = "draft";
        created.assigneeUserId = assigneeId;
        created.proposedBy = request.operatorUserId;
        created.note = request.note.substr(0, 1000);
        created.rowVersion = 1;

        insertWorkflowAudit(
            *session, proposalId, "compensation_created", request.operatorUserId,
            created.branchId, created.departmentId, nlohmann::json::object(),
            auditSnapshot(created, &(*employment)), request.note);

        session->sql("COMMIT").execute();

        OpResult ok;
        ok.ok = true;
        ok.httpStatus = 201;
        ok.message = "薪酬提案已创建";
        ok.data = proposalToJson(created, true, employment->userId, employment->status);
        return ok;
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        const std::string what = e.what();
        if (what.find("Duplicate") != std::string::npos ||
            what.find("uq_compensation_proposal_open") != std::string::npos)
        {
            return fail(409, "同阶段已有进行中的薪酬案件", "OPEN_PROPOSAL_EXISTS");
        }
        std::cerr << "CompensationWorkflowService::createProposal failed: " << e.what()
                  << std::endl;
        return fail(500, std::string("创建失败: ") + e.what());
    }
}

OpResult updateProposal(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const UpdateRequest &request)
{
    if (!dbManager || !dbManager->getSession())
    {
        return fail(500, "数据库不可用");
    }
    if (request.operatorUserId <= 0 || request.proposalId <= 0)
    {
        return fail(400, "参数无效", "INVALID_REQUEST");
    }
    if (!request.hasExpectedRowVersion || request.expectedRowVersion < 1)
    {
        return fail(400, "expectedRowVersion 必填", "EXPECTED_ROW_VERSION_REQUIRED");
    }
    if (!hasPermission(dbManager, request.operatorUserId, Permissions::kCompensationPropose))
    {
        return fail(403, "缺少薪酬拟案权限", "PROPOSE_DENIED");
    }

    PayFields pay;
    std::string payError;
    if (!normalizePayFields(
            request.payType, request.baseSalary, request.hourlyRate,
            request.hasSocial ? request.socialInsuranceHousingFund : 0.0, pay, payError))
    {
        return fail(400, payError, "INVALID_PAY");
    }
    if (!request.effectiveFrom.empty() && !isValidDate(request.effectiveFrom))
    {
        return fail(400, "effectiveFrom 格式无效", "INVALID_DATE");
    }

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        auto proposal = lockProposal(*session, request.proposalId);
        if (!proposal.has_value())
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        // v6：permission + assignee + 提案组织快照范围；范围外 fail closed 为 404
        {
            const auto scope =
                RbacService::loadEffectiveOrgScope(dbManager, request.operatorUserId);
            if (!departmentInScope(scope, proposal->departmentId))
            {
                rollbackTransactionQuietly(*session);
                return fail(404, "提案不存在", "NOT_FOUND");
            }
        }
        if (proposal->status == "active" || proposal->status == "cancelled")
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "已结束的提案不可修改", "IMMUTABLE_STATUS");
        }
        if (proposal->status != "draft" && proposal->status != "returned")
        {
            // 金额提交后冻结
            rollbackTransactionQuietly(*session);
            return fail(409, "提交后金额已冻结，仅 draft/returned 可编辑", "AMOUNT_FROZEN");
        }
        if (proposal->assigneeUserId != request.operatorUserId)
        {
            rollbackTransactionQuietly(*session);
            return fail(403, "仅案件受理人可编辑", "NOT_ASSIGNEE");
        }
        if (proposal->rowVersion != request.expectedRowVersion)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }

        const std::string effectiveFrom =
            request.effectiveFrom.empty() ? proposal->effectiveFrom : request.effectiveFrom;
        const std::string note =
            request.hasNote ? request.note.substr(0, 1000) : proposal->note;
        const double social =
            request.hasSocial ? request.socialInsuranceHousingFund : proposal->social;
        if (!normalizePayFields(
                request.payType, request.baseSalary, request.hourlyRate, social, pay, payError))
        {
            rollbackTransactionQuietly(*session);
            return fail(400, payError, "INVALID_PAY");
        }

        nlohmann::json before = auditSnapshot(*proposal);
        auto upd = session
                       ->sql("UPDATE compensation_proposal SET pay_type = ?, base_salary = ?, "
                             "hourly_rate = ?, social_insurance_housing_fund = ?, effective_from = ?, "
                             "note = ?, row_version = row_version + 1 "
                             "WHERE id = ? AND status IN ('draft','returned') AND row_version = ?")
                       .bind(pay.payType,
                             pay.baseSalary.has_value() ? mysqlx::Value(*pay.baseSalary)
                                                        : mysqlx::Value(),
                             pay.hourlyRate.has_value() ? mysqlx::Value(*pay.hourlyRate)
                                                        : mysqlx::Value(),
                             pay.social, effectiveFrom, note, request.proposalId,
                             request.expectedRowVersion)
                       .execute();
        if (upd.getAffectedItemsCount() != 1)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }

        proposal->payType = pay.payType;
        proposal->baseSalary = pay.baseSalary;
        proposal->hourlyRate = pay.hourlyRate;
        proposal->social = pay.social;
        proposal->effectiveFrom = effectiveFrom;
        proposal->note = note;
        proposal->rowVersion = request.expectedRowVersion + 1;

        insertWorkflowAudit(
            *session, proposal->id, "compensation_updated", request.operatorUserId,
            proposal->branchId, proposal->departmentId, before, auditSnapshot(*proposal), note);

        session->sql("COMMIT").execute();
        OpResult ok;
        ok.ok = true;
        ok.httpStatus = 200;
        ok.message = "薪酬提案已更新";
        ok.data = proposalToJson(*proposal, true);
        return ok;
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        std::cerr << "CompensationWorkflowService::updateProposal failed: " << e.what()
                  << std::endl;
        return fail(500, std::string("更新失败: ") + e.what());
    }
}

OpResult submitProposal(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const IdVersionRequest &request)
{
    if (!dbManager || !dbManager->getSession())
    {
        return fail(500, "数据库不可用");
    }
    if (request.operatorUserId <= 0 || request.proposalId <= 0)
    {
        return fail(400, "参数无效", "INVALID_REQUEST");
    }
    if (!request.hasExpectedRowVersion || request.expectedRowVersion < 1)
    {
        return fail(400, "expectedRowVersion 必填", "EXPECTED_ROW_VERSION_REQUIRED");
    }
    if (!hasPermission(dbManager, request.operatorUserId, Permissions::kCompensationPropose))
    {
        return fail(403, "缺少薪酬拟案权限", "PROPOSE_DENIED");
    }

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        auto proposal = lockProposal(*session, request.proposalId);
        if (!proposal.has_value())
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        // v6：permission + assignee + 提案组织快照范围；范围外 fail closed 为 404
        {
            const auto scope =
                RbacService::loadEffectiveOrgScope(dbManager, request.operatorUserId);
            if (!departmentInScope(scope, proposal->departmentId))
            {
                rollbackTransactionQuietly(*session);
                return fail(404, "提案不存在", "NOT_FOUND");
            }
        }
        if (proposal->status != "draft" && proposal->status != "returned")
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "仅 draft/returned 可提交", "INVALID_STATUS");
        }
        if (proposal->assigneeUserId != request.operatorUserId)
        {
            rollbackTransactionQuietly(*session);
            return fail(403, "仅案件受理人可提交", "NOT_ASSIGNEE");
        }
        if (proposal->rowVersion != request.expectedRowVersion)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }

        nlohmann::json before = auditSnapshot(*proposal);
        auto upd = session
                       ->sql("UPDATE compensation_proposal SET status = 'submitted', "
                             "submitted_by = ?, submitted_at = NOW(), "
                             "approved_by = NULL, approved_at = NULL, "
                             "finance_confirmed_by = NULL, finance_confirmed_at = NULL, "
                             "expected_employment_row_version = NULL, "
                             "row_version = row_version + 1 "
                             "WHERE id = ? AND status IN ('draft','returned') AND row_version = ?")
                       .bind(request.operatorUserId, request.proposalId, request.expectedRowVersion)
                       .execute();
        if (upd.getAffectedItemsCount() != 1)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }

        proposal->status = "submitted";
        proposal->submittedBy = request.operatorUserId;
        proposal->approvedBy = 0;
        proposal->financeConfirmedBy = 0;
        proposal->hasExpectedEmploymentRowVersion = false;
        proposal->rowVersion = request.expectedRowVersion + 1;

        insertWorkflowAudit(
            *session, proposal->id, "compensation_submitted", request.operatorUserId,
            proposal->branchId, proposal->departmentId, before, auditSnapshot(*proposal),
            request.reason);

        session->sql("COMMIT").execute();
        OpResult ok;
        ok.ok = true;
        ok.httpStatus = 200;
        ok.message = "薪酬提案已提交";
        ok.data = proposalToJson(*proposal, true);
        return ok;
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        std::cerr << "CompensationWorkflowService::submitProposal failed: " << e.what()
                  << std::endl;
        return fail(500, std::string("提交失败: ") + e.what());
    }
}

OpResult reassignProposal(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const ReassignRequest &request)
{
    if (!dbManager || !dbManager->getSession())
    {
        return fail(500, "数据库不可用");
    }
    if (request.operatorUserId <= 0 || request.proposalId <= 0 ||
        request.targetAssigneeUserId <= 0)
    {
        return fail(400, "参数无效", "INVALID_REQUEST");
    }
    if (!request.hasExpectedRowVersion || request.expectedRowVersion < 1)
    {
        return fail(400, "expectedRowVersion 必填", "EXPECTED_ROW_VERSION_REQUIRED");
    }
    if (request.reason.empty())
    {
        return fail(400, "reason 不能为空", "REASON_REQUIRED");
    }
    if (!hasPermission(dbManager, request.operatorUserId, Permissions::kCompensationReassignCase))
    {
        return fail(403, "缺少案件改派权限", "REASSIGN_DENIED");
    }

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        auto proposal = lockProposal(*session, request.proposalId);
        if (!proposal.has_value())
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        if (!isOpenStatus(proposal->status))
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "已结束的案件不可改派", "IMMUTABLE_STATUS");
        }
        if (proposal->rowVersion != request.expectedRowVersion)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }

        const auto opScope =
            RbacService::loadEffectiveOrgScope(dbManager, request.operatorUserId);
        if (!departmentInScope(opScope, proposal->departmentId))
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        if (!userCanProposeOnDepartment(
                dbManager, *session, request.targetAssigneeUserId, proposal->departmentId))
        {
            rollbackTransactionQuietly(*session);
            return fail(400, "目标受理人权限或范围不满足", "ASSIGNEE_INVALID");
        }

        nlohmann::json before = auditSnapshot(*proposal);
        auto upd = session
                       ->sql(std::string("UPDATE compensation_proposal SET assignee_user_id = ?, "
                                         "row_version = row_version + 1 "
                                         "WHERE id = ? AND row_version = ? AND status IN (") +
                             kOpenStatusSqlIn + ")")
                       .bind(request.targetAssigneeUserId, request.proposalId,
                             request.expectedRowVersion)
                       .execute();
        if (upd.getAffectedItemsCount() != 1)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }

        proposal->assigneeUserId = request.targetAssigneeUserId;
        proposal->rowVersion = request.expectedRowVersion + 1;

        insertWorkflowAudit(
            *session, proposal->id, "compensation_reassigned", request.operatorUserId,
            proposal->branchId, proposal->departmentId, before, auditSnapshot(*proposal),
            request.reason);

        session->sql("COMMIT").execute();
        OpResult ok;
        ok.ok = true;
        ok.httpStatus = 200;
        ok.message = "案件已改派";
        ok.data = proposalToJson(*proposal, canSeeAmounts(dbManager, request.operatorUserId, *proposal));
        return ok;
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        std::cerr << "CompensationWorkflowService::reassignProposal failed: " << e.what()
                  << std::endl;
        return fail(500, std::string("改派失败: ") + e.what());
    }
}

OpResult decideProposal(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const DecisionRequest &request)
{
    if (!dbManager || !dbManager->getSession())
    {
        return fail(500, "数据库不可用");
    }
    if (request.operatorUserId <= 0 || request.proposalId <= 0)
    {
        return fail(400, "参数无效", "INVALID_REQUEST");
    }
    if (!request.hasExpectedRowVersion || request.expectedRowVersion < 1)
    {
        return fail(400, "expectedRowVersion 必填", "EXPECTED_ROW_VERSION_REQUIRED");
    }
    if (request.reason.empty())
    {
        return fail(400, "reason 不能为空", "REASON_REQUIRED");
    }
    if (!hasPermission(dbManager, request.operatorUserId, Permissions::kCompensationApprove))
    {
        return fail(403, "缺少薪酬审批权限", "APPROVE_DENIED");
    }

    const bool isApprove = request.action == DecisionAction::Approve;
    const auto scope = RbacService::loadEffectiveOrgScope(dbManager, request.operatorUserId);

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        // 固定锁顺序：proposal → employment
        auto proposal = lockProposal(*session, request.proposalId);
        if (!proposal.has_value())
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        if (!departmentInScope(scope, proposal->departmentId))
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        if (proposal->status != "submitted")
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "仅 submitted 状态可审批", "INVALID_STATUS");
        }
        if (proposal->rowVersion != request.expectedRowVersion)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }
        // proposed_by 不能自批
        if (proposal->proposedBy > 0 && proposal->proposedBy == request.operatorUserId)
        {
            rollbackTransactionQuietly(*session);
            return fail(403, "不能审批自己发起的薪酬提案", "SELF_APPROVAL");
        }

        auto employment = lockEmploymentById(*session, proposal->employmentId);
        if (!employment.has_value() || employment->isDeleted != 0)
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }

        nlohmann::json before = auditSnapshot(*proposal, &(*employment));

        if (!isApprove)
        {
            auto upd = session
                           ->sql("UPDATE compensation_proposal SET status = 'returned', "
                                 "row_version = row_version + 1 "
                                 "WHERE id = ? AND status = 'submitted' AND row_version = ?")
                           .bind(request.proposalId, request.expectedRowVersion)
                           .execute();
            if (upd.getAffectedItemsCount() != 1)
            {
                rollbackTransactionQuietly(*session);
                return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
            }
            proposal->status = "returned";
            proposal->rowVersion = request.expectedRowVersion + 1;

            insertWorkflowAudit(
                *session, proposal->id, "compensation_returned", request.operatorUserId,
                proposal->branchId, proposal->departmentId, before,
                auditSnapshot(*proposal, &(*employment)), request.reason);

            session->sql("COMMIT").execute();
            OpResult ok;
            ok.ok = true;
            ok.httpStatus = 200;
            ok.message = "已退回薪酬提案";
            ok.data = proposalToJson(*proposal, true, employment->userId, employment->status);
            ok.data["action"] = "return";
            return ok;
        }

        std::string phaseCode;
        std::string phaseMsg;
        if (!phasePrerequisiteOk(*session, proposal->phase, *employment, phaseCode, phaseMsg))
        {
            rollbackTransactionQuietly(*session);
            return fail(409, phaseMsg, phaseCode);
        }
        std::string dateCode;
        std::string dateMsg;
        if (!validateEffectiveDateBoundary(
                *session, *proposal, *employment, dateCode, dateMsg))
        {
            rollbackTransactionQuietly(*session);
            return fail(409, dateMsg, dateCode.empty() ? "EFFECTIVE_DATE_CONFLICT" : dateCode);
        }

        // 批准：写当时 employment.row_version 到 expected_employment_row_version
        auto upd = session
                       ->sql("UPDATE compensation_proposal SET status = 'management_approved', "
                             "approved_by = ?, approved_at = NOW(), "
                             "expected_employment_row_version = ?, "
                             "row_version = row_version + 1 "
                             "WHERE id = ? AND status = 'submitted' AND row_version = ? "
                             "AND proposed_by <> ?")
                       .bind(request.operatorUserId, employment->rowVersion, request.proposalId,
                             request.expectedRowVersion, request.operatorUserId)
                       .execute();
        if (upd.getAffectedItemsCount() != 1)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }

        proposal->status = "management_approved";
        proposal->approvedBy = request.operatorUserId;
        proposal->expectedEmploymentRowVersion = employment->rowVersion;
        proposal->hasExpectedEmploymentRowVersion = true;
        proposal->rowVersion = request.expectedRowVersion + 1;

        insertWorkflowAudit(
            *session, proposal->id, "compensation_management_approved", request.operatorUserId,
            proposal->branchId, proposal->departmentId, before,
            auditSnapshot(*proposal, &(*employment)), request.reason);

        session->sql("COMMIT").execute();
        OpResult ok;
        ok.ok = true;
        ok.httpStatus = 200;
        ok.message = "已批准薪酬提案";
        ok.data = proposalToJson(*proposal, true, employment->userId, employment->status);
        ok.data["action"] = "approve";
        return ok;
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        const std::string what = e.what();
        if (what.find("chk_cp_sod") != std::string::npos)
        {
            return fail(403, "违反职责分离约束", "SEPARATION_OF_DUTIES");
        }
        std::cerr << "CompensationWorkflowService::decideProposal failed: " << e.what()
                  << std::endl;
        return fail(500, std::string("审批失败: ") + e.what());
    }
}

OpResult confirmActivation(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const FinanceConfirmRequest &request)
{
    if (!dbManager || !dbManager->getSession())
    {
        return fail(500, "数据库不可用");
    }
    if (request.operatorUserId <= 0 || request.proposalId <= 0)
    {
        return fail(400, "参数无效", "INVALID_REQUEST");
    }
    if (!request.hasExpectedRowVersion || request.expectedRowVersion < 1)
    {
        return fail(400, "expectedRowVersion 必填", "EXPECTED_ROW_VERSION_REQUIRED");
    }
    // 精确 salary-profile:activate，不接受 salary:write 旁路
    if (!hasPermission(dbManager, request.operatorUserId, Permissions::kSalaryProfileActivate))
    {
        return fail(403, "缺少薪资配置激活权限", "ACTIVATE_DENIED");
    }

    const auto scope = RbacService::loadEffectiveOrgScope(dbManager, request.operatorUserId);

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        // 固定锁顺序：proposal → employment → salaryProfile
        auto proposal = lockProposal(*session, request.proposalId);
        if (!proposal.has_value())
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        if (!departmentInScope(scope, proposal->departmentId))
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        if (proposal->status != "management_approved")
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "仅 management_approved 可财务确认", "INVALID_STATUS");
        }
        if (proposal->rowVersion != request.expectedRowVersion)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }
        if (!proposal->hasExpectedEmploymentRowVersion)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "缺少任职版本快照", "MISSING_EMPLOYMENT_VERSION");
        }
        // 财务确认人不得是 proposed_by 或 approved_by
        if (proposal->proposedBy == request.operatorUserId ||
            proposal->approvedBy == request.operatorUserId)
        {
            rollbackTransactionQuietly(*session);
            return fail(403, "财务确认人不得为拟案人或管理审批人", "SEPARATION_OF_DUTIES");
        }

        auto employment = lockEmploymentById(*session, proposal->employmentId);
        if (!employment.has_value() || employment->isDeleted != 0)
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        if (employment->status == "separated" || employment->status == "rejected")
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "不能将已结束任职翻回在职", "EMPLOYMENT_STATE_CONFLICT");
        }
        // 双 CAS：employment.row_version 必须等于批准时快照
        if (employment->rowVersion != proposal->expectedEmploymentRowVersion)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "员工任职版本已变更，请刷新后重试", "STALE_VERSION");
        }

        std::string phaseCode;
        std::string phaseMsg;
        if (!phasePrerequisiteOk(*session, proposal->phase, *employment, phaseCode, phaseMsg))
        {
            rollbackTransactionQuietly(*session);
            return fail(409, phaseMsg, phaseCode.empty() ? "EMPLOYMENT_STATE_CONFLICT" : phaseCode);
        }
        std::string dateCode;
        std::string dateMsg;
        if (!validateEffectiveDateBoundary(
                *session, *proposal, *employment, dateCode, dateMsg))
        {
            rollbackTransactionQuietly(*session);
            return fail(409, dateMsg, dateCode.empty() ? "EFFECTIVE_DATE_CONFLICT" : dateCode);
        }

        nlohmann::json before = auditSnapshot(*proposal, &(*employment));

        // 锁并物化全部 salaryProfile；拒绝未来/重叠区间；关闭 open 行须 affected=1
        std::string profileCode;
        std::string profileMsg;
        if (!prepareSalaryProfileActivation(
                *session, employment->userId, proposal->effectiveFrom, profileCode, profileMsg))
        {
            rollbackTransactionQuietly(*session);
            return fail(
                409, profileMsg,
                profileCode.empty() ? "SALARY_PROFILE_OVERLAP" : profileCode);
        }

        auto profileIns =
            session
                ->sql("INSERT INTO salaryProfile "
                      "(user_id, pay_type, base_salary, hourly_rate, "
                      "social_insurance_housing_fund, effective_from, effective_to) "
                      "VALUES (?, ?, ?, ?, ?, ?, NULL)")
                .bind(employment->userId, proposal->payType,
                      proposal->baseSalary.has_value() ? mysqlx::Value(*proposal->baseSalary)
                                                       : mysqlx::Value(),
                      proposal->hourlyRate.has_value() ? mysqlx::Value(*proposal->hourlyRate)
                                                       : mysqlx::Value(),
                      proposal->social, proposal->effectiveFrom)
                .execute();
        const int newProfileId = static_cast<int>(profileIns.getAutoIncrementValue());

        // 任职状态迁移 + CAS expected_employment_row_version
        long long empAffected = 0;
        if (proposal->phase == "probation")
        {
            empAffected =
                session
                    ->sql("UPDATE employment SET status = 'probation', "
                          "probation_start = COALESCE(probation_start, ?), "
                          "row_version = row_version + 1 "
                          "WHERE id = ? AND status = 'onboarding' AND row_version = ? "
                          "AND COALESCE(probation_waived, 0) = 0")
                    .bind(proposal->effectiveFrom, employment->id,
                          proposal->expectedEmploymentRowVersion)
                    .execute()
                    .getAffectedItemsCount();
        }
        else if (proposal->phase == "regular")
        {
            if (employment->probationWaived == 1)
            {
                empAffected =
                    session
                        ->sql("UPDATE employment SET status = 'active', "
                              "regularized_at = COALESCE(regularized_at, NOW()), "
                              "row_version = row_version + 1 "
                              "WHERE id = ? AND status = 'onboarding' AND row_version = ? "
                              "AND COALESCE(probation_waived, 0) = 1")
                        .bind(employment->id, proposal->expectedEmploymentRowVersion)
                        .execute()
                        .getAffectedItemsCount();
            }
            else
            {
                empAffected =
                    session
                        ->sql("UPDATE employment SET status = 'active', "
                              "regularized_at = COALESCE(regularized_at, NOW()), "
                              "row_version = row_version + 1 "
                              "WHERE id = ? AND status = 'regularization_pending' "
                              "AND row_version = ?")
                        .bind(employment->id, proposal->expectedEmploymentRowVersion)
                        .execute()
                        .getAffectedItemsCount();
            }
        }
        else
        {
            // adjustment：保持 active，仍递增 row_version
            empAffected =
                session
                    ->sql("UPDATE employment SET row_version = row_version + 1 "
                          "WHERE id = ? AND status = 'active' AND row_version = ?")
                    .bind(employment->id, proposal->expectedEmploymentRowVersion)
                    .execute()
                    .getAffectedItemsCount();
        }
        if (empAffected != 1)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "员工任职状态或版本已变更", "EMPLOYMENT_STATE_CONFLICT");
        }

        auto propUpd =
            session
                ->sql("UPDATE compensation_proposal SET status = 'active', "
                      "finance_confirmed_by = ?, finance_confirmed_at = NOW(), "
                      "salary_profile_id = ?, row_version = row_version + 1 "
                      "WHERE id = ? AND status = 'management_approved' AND row_version = ? "
                      "AND proposed_by <> ? AND approved_by <> ?")
                .bind(request.operatorUserId, newProfileId, request.proposalId,
                      request.expectedRowVersion, request.operatorUserId, request.operatorUserId)
                .execute();
        if (propUpd.getAffectedItemsCount() != 1)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }

        proposal->status = "active";
        proposal->financeConfirmedBy = request.operatorUserId;
        proposal->salaryProfileId = newProfileId;
        proposal->rowVersion = request.expectedRowVersion + 1;
        employment->rowVersion = proposal->expectedEmploymentRowVersion + 1;
        if (proposal->phase == "probation")
        {
            employment->status = "probation";
        }
        else if (proposal->phase == "regular")
        {
            employment->status = "active";
        }

        insertWorkflowAudit(
            *session, proposal->id, "compensation_finance_confirmed", request.operatorUserId,
            proposal->branchId, proposal->departmentId, before,
            auditSnapshot(*proposal, &(*employment)), request.reason);

        session->sql("COMMIT").execute();
        OpResult ok;
        ok.ok = true;
        ok.httpStatus = 200;
        ok.message = "薪酬已激活";
        ok.data = proposalToJson(*proposal, true, employment->userId, employment->status);
        ok.data["salaryProfileId"] = newProfileId;
        return ok;
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        const std::string what = e.what();
        if (what.find("chk_cp_sod") != std::string::npos)
        {
            return fail(403, "违反职责分离约束", "SEPARATION_OF_DUTIES");
        }
        if (what.find("Duplicate") != std::string::npos ||
            what.find("uq_salaryProfile") != std::string::npos)
        {
            return fail(409, "薪资配置生效区间冲突", "SALARY_PROFILE_OVERLAP");
        }
        std::cerr << "CompensationWorkflowService::confirmActivation failed: " << e.what()
                  << std::endl;
        return fail(500, std::string("激活失败: ") + e.what());
    }
}

OpResult returnActivation(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const FinanceConfirmRequest &request)
{
    if (!dbManager || !dbManager->getSession())
    {
        return fail(500, "数据库不可用");
    }
    if (request.operatorUserId <= 0 || request.proposalId <= 0)
    {
        return fail(400, "参数无效", "INVALID_REQUEST");
    }
    if (!request.hasExpectedRowVersion || request.expectedRowVersion < 1)
    {
        return fail(400, "expectedRowVersion 必填", "EXPECTED_ROW_VERSION_REQUIRED");
    }
    if (request.reason.empty())
    {
        return fail(400, "reason 不能为空", "REASON_REQUIRED");
    }
    if (!hasPermission(dbManager, request.operatorUserId, Permissions::kSalaryProfileActivate))
    {
        return fail(403, "缺少薪资配置激活权限", "ACTIVATE_DENIED");
    }

    const auto scope = RbacService::loadEffectiveOrgScope(dbManager, request.operatorUserId);

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        auto proposal = lockProposal(*session, request.proposalId);
        if (!proposal.has_value())
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        if (!departmentInScope(scope, proposal->departmentId))
        {
            rollbackTransactionQuietly(*session);
            return fail(404, "提案不存在", "NOT_FOUND");
        }
        if (proposal->status != "management_approved")
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "仅 management_approved 可财务退回", "INVALID_STATUS");
        }
        if (proposal->rowVersion != request.expectedRowVersion)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }

        nlohmann::json before = auditSnapshot(*proposal);
        auto upd = session
                       ->sql("UPDATE compensation_proposal SET status = 'returned', "
                             "row_version = row_version + 1 "
                             "WHERE id = ? AND status = 'management_approved' AND row_version = ?")
                       .bind(request.proposalId, request.expectedRowVersion)
                       .execute();
        if (upd.getAffectedItemsCount() != 1)
        {
            rollbackTransactionQuietly(*session);
            return fail(409, "提案版本已变更，请刷新后重试", "STALE_VERSION");
        }

        proposal->status = "returned";
        proposal->rowVersion = request.expectedRowVersion + 1;

        insertWorkflowAudit(
            *session, proposal->id, "compensation_finance_returned", request.operatorUserId,
            proposal->branchId, proposal->departmentId, before, auditSnapshot(*proposal),
            request.reason);

        // 不创建 salaryProfile、不改 employment
        session->sql("COMMIT").execute();
        OpResult ok;
        ok.ok = true;
        ok.httpStatus = 200;
        ok.message = "已退回薪酬提案";
        ok.data = proposalToJson(*proposal, true);
        return ok;
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        std::cerr << "CompensationWorkflowService::returnActivation failed: " << e.what()
                  << std::endl;
        return fail(500, std::string("退回失败: ") + e.what());
    }
}
}
