#include "personnelHandler.h"
#include "../user/userPhoneSync/userPhoneSync.h"
#include "../../../services/employment/EmploymentAssignmentService.h"
#include "../../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../../services/rbac/RbacService.h"
#include "../../../utils/permissions/Permissions.h"
#include "../../../utils/requestUtils/RequestUtils.h"

#include <sstream>

namespace
{
using RequestUtils::getJsonInt;
using RequestUtils::getJsonString;
using RequestUtils::normalizePage;
using RequestUtils::normalizePageSize;

std::string joinIds(const std::vector<int> &ids)
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

std::string orgScopeConditionForUser(
    int userId,
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const std::string &departmentColumn)
{
    if (userId <= 0)
    {
        return " AND 1 = 0 ";
    }
    const RbacService::EffectiveOrgScope scope =
        RbacService::loadEffectiveOrgScope(dbManager, userId);
    if (scope.unrestricted)
    {
        return "";
    }
    if (scope.departmentIds.empty())
    {
        return " AND 1 = 0 ";
    }
    return " AND " + departmentColumn + " IN (" + joinIds(scope.departmentIds) + ") ";
}

// 隐藏超管职位、持 rbac:manage（职位或个人）、已删除账号。
std::string highPrivilegeExcludeSql(const std::string &userAlias = "u", const std::string &posAlias = "pos")
{
    return " AND " + userAlias + ".is_deleted = 0 "
           " AND COALESCE(" + posAlias + ".system_key, '') <> 'super-admin' "
           " AND NOT EXISTS ("
           "   SELECT 1 FROM position_permissions pp "
           "   WHERE pp.position_id = " + userAlias + ".position_id "
           "     AND pp.permission_key = 'rbac:manage'"
           " ) "
           " AND NOT EXISTS ("
           "   SELECT 1 FROM user_permissions up "
           "   WHERE up.user_id = " + userAlias + ".id "
           "     AND up.permission_key = 'rbac:manage'"
           " ) ";
}

crow::response assignmentResultToResponse(
    const crow::request &req,
    const EmploymentAssignmentService::AssignResult &result)
{
    if (!result.ok)
    {
        if (result.httpStatus == 404)
        {
            return ResponseHelper::notFound(req, result.message);
        }
        if (result.httpStatus == 403)
        {
            return ResponseHelper::permission_denied(req, result.message, result.errorCode);
        }
        if (result.httpStatus == 409)
        {
            return ResponseHelper::fail(
                req, 409, ResponseCode::BusinessConflict, result.message,
                ResponseErrorType::BusinessConflict, result.errorCode.empty() ? result.message : result.errorCode);
        }
        if (result.httpStatus >= 500)
        {
            return ResponseHelper::system_error(req, result.message);
        }
        return ResponseHelper::validation(req, result.message);
    }

    return ResponseHelper::success(req, {
        {"user_id", result.userId},
        {"position_id", result.positionId > 0 ? nlohmann::json(result.positionId) : nlohmann::json(nullptr)},
        {"account_type", result.accountType},
        {"assignment_status", result.assignmentStatus},
        {"assignment_id", result.assignmentId},
        {"employment_id", result.employmentId},
        {"message", result.message},
    });
}
}

crow::response personnelHandler::createUser(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string name = request_body.value("name", "");
        std::string phone = request_body.value("phone", "");
        std::string email = request_body.value("email", "");
        std::string password = request_body.value("password", "");
        std::string birthday = request_body.value("birthday", "1970-01-01");
        std::string head_image = request_body.value("head_image", "");

        if (name.empty())
        {
            return ResponseHelper::validation(req, "用户名不能为空");
        }

        if (phone.empty() && email.empty())
        {
            return ResponseHelper::validation(req, "手机号和邮箱至少填写一项");
        }

        if (password.empty())
        {
            password = "123456";
        }

        if (!phone.empty())
        {
            mysqlx::SqlResult phone_result = dbManager->getSession()
                                                 ->sql("SELECT user_id FROM phones WHERE phone = ?")
                                                 .bind(phone)
                                                 .execute();

            if (phone_result.count() > 0)
            {
                return ResponseHelper::validation(req, "手机号已存在");
            }
        }

        if (!email.empty())
        {
            mysqlx::SqlResult email_result = dbManager->getSession()
                                                 ->sql("SELECT id FROM users WHERE email = ?")
                                                 .bind(email)
                                                 .execute();

            if (email_result.count() > 0)
            {
                return ResponseHelper::validation(req, "邮箱已存在");
            }
        }

        const std::string hashed_password = hash_password(password);

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        mysqlx::SqlResult result;
        try
        {
            result = session->sql("INSERT INTO users (account_type, position_id, name, password, email, birthday, head_image) "
                                  "VALUES ('customer', NULL, ?, ?, ?, ?, ?)")
                         .bind(name, hashed_password, email, birthday, head_image)
                         .execute();

            if (result.getAffectedItemsCount() == 0)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::system_error(req, "创建失败");
            }

            if (!UserPhoneSync::upsertUserPhone(*session, static_cast<int>(result.getAutoIncrementValue()), phone))
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::system_error(req, "用户创建失败，手机号同步未完成");
            }

            session->sql("COMMIT").execute();
            AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
        }
        catch (...)
        {
            rollbackTransactionQuietly(*session);
            throw;
        }

        nlohmann::json payload;
        payload["success"] = true;
        payload["message"] = "创建成功";
        payload["data"] = {
            {"id", result.getAutoIncrementValue()},
            {"type_id", 0},
            {"type_name", "普通用户"},
            {"name", name},
            {"phone", phone},
            {"email", email},
            {"birthday", birthday},
            {"head_image", head_image},
        };

        return ResponseHelper::created(req, payload);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response personnelHandler::deleteUser(const crow::request &req, int &userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userID = request_body.value("user_id", 0);
        if (userID == 0)
        {
            return ResponseHelper::validation(req, "用户ID不能为空");
        }

        if (userID == userId)
        {
            return ResponseHelper::validation(req, "不能删除当前登录的超级管理员");
        }

        mysqlx::SqlResult target_result = dbManager->getSession()
                                              ->sql("SELECT account_type FROM users WHERE id = ? AND is_deleted = 0")
                                              .bind(userID)
                                              .execute();

        if (target_result.count() == 0)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        mysqlx::Row target_row = target_result.fetchOne();
        const std::string accountType = target_row[0].isNull() ? "" : target_row[0].get<std::string>();
        if (accountType != "customer")
        {
            return ResponseHelper::unavailable(req, "这里只能删除普通用户");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("UPDATE users "
                                             "SET is_deleted = 1, deleted_at = NOW(), deleted_by = ? "
                                             "WHERE id = ? AND account_type = 'customer' AND is_deleted = 0")
                                       .bind(userId, userID)
                                       .execute();

        if (result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
        return ResponseHelper::success(req, "删除成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response personnelHandler::searchEmployees(
    const crow::request &req,
    const nlohmann::json &body,
    int operatorUserId)
{
    try
    {
        const std::string keyword = getJsonString(body, "keyword");
        const std::string likeKeyword = "%" + keyword + "%";
        const int page = normalizePage(getJsonInt(body, "page", 1));
        const int pageSize = normalizePageSize(getJsonInt(body, "pageSize", 10), 10, 100);
        const int offset = (page - 1) * pageSize;

        // B12: 无 candidate scope 模型时，仅 unrestricted 人事可见未任职 customer。
        // 受限人事只能见组织范围内的 staff，不能搜索/猜 ID 读无归属 customer。
        const RbacService::EffectiveOrgScope scope =
            RbacService::loadEffectiveOrgScope(dbManager, operatorUserId);
        std::string scopeFilter;
        if (scope.unrestricted)
        {
            scopeFilter = "";
        }
        else if (scope.departmentIds.empty())
        {
            scopeFilter = " AND 1 = 0 ";
        }
        else
        {
            scopeFilter = " AND u.account_type = 'staff' AND pos.department_id IN (" +
                          joinIds(scope.departmentIds) + ") ";
        }

        const std::string hideHigh = highPrivilegeExcludeSql("u", "pos");

        auto listQuery = dbManager->getSession()
                             ->sql(std::string(
                                       "SELECT u.id, COALESCE(u.position_id, 0), "
                                       "CASE WHEN u.account_type = 'customer' THEN '普通用户' ELSE COALESCE(pos.name, '') END, "
                                       "u.name, ph.phone, u.email, "
                                       "COALESCE(pos.staff_kind, ''), COALESCE(pos.assignment_policy, ''), "
                                       "COALESCE(d.name, ''), COALESCE(u.account_type, '') "
                                       "FROM users u "
                                       "LEFT JOIN positions pos ON pos.id = u.position_id "
                                       "LEFT JOIN departments d ON d.id = pos.department_id "
                                       "LEFT JOIN phones ph ON ph.user_id = u.id "
                                       "WHERE 1=1 ") +
                                   hideHigh + scopeFilter +
                                   " AND (? = '' OR COALESCE(u.name, '') LIKE ? OR COALESCE(ph.phone, '') LIKE ? "
                                   "      OR COALESCE(u.email, '') LIKE ?) "
                                   "ORDER BY u.id ASC LIMIT ?, ?")
                             .bind(keyword, likeKeyword, likeKeyword, likeKeyword, offset, pageSize);
        mysqlx::SqlResult result = listQuery.execute();

        auto countQuery = dbManager->getSession()
                              ->sql(std::string(
                                        "SELECT COUNT(DISTINCT u.id) FROM users u "
                                        "LEFT JOIN positions pos ON pos.id = u.position_id "
                                        "LEFT JOIN phones ph ON ph.user_id = u.id "
                                        "WHERE 1=1 ") +
                                    hideHigh + scopeFilter +
                                    " AND (? = '' OR COALESCE(u.name, '') LIKE ? OR COALESCE(ph.phone, '') LIKE ? "
                                    "      OR COALESCE(u.email, '') LIKE ?)")
                              .bind(keyword, likeKeyword, likeKeyword, likeKeyword);
        const int total = countQuery.execute().fetchOne()[0].get<int>();

        nlohmann::json items = nlohmann::json::array();
        for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
        {
            items.push_back({
                {"id", row[0].get<int>()},
                {"position_id", row[1].isNull() || row[1].get<int>() == 0
                                    ? nlohmann::json(nullptr)
                                    : nlohmann::json(row[1].get<int>())},
                {"position_name", row[2].isNull() ? "" : row[2].get<std::string>()},
                {"name", row[3].isNull() ? "" : clean_string(row[3].get<std::string>())},
                {"phone", row[4].isNull() ? "" : clean_string(row[4].get<std::string>())},
                {"email", row[5].isNull() ? "" : clean_string(row[5].get<std::string>())},
                {"staff_kind", row[6].isNull() ? "" : row[6].get<std::string>()},
                {"assignment_policy", row[7].isNull() ? "" : row[7].get<std::string>()},
                {"department_name", row[8].isNull() ? "" : row[8].get<std::string>()},
                {"account_type", row[9].isNull() ? "" : row[9].get<std::string>()},
            });
        }

        return ResponseHelper::success(req, {
            {"items", items},
            {"total", total},
            {"page", page},
            {"pageSize", pageSize},
        });
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response personnelHandler::getEmployee(
    const crow::request &req,
    int operatorUserId,
    int employeeId)
{
    try
    {
        if (employeeId <= 0)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        const std::string hideHigh = highPrivilegeExcludeSql("u", "pos");
        const RbacService::EffectiveOrgScope scope =
            RbacService::loadEffectiveOrgScope(dbManager, operatorUserId);
        // B12: 受限 fail-closed — 不可靠猜 ID 读 customer 或范围外 staff
        std::string visibleFilter;
        if (scope.unrestricted)
        {
            visibleFilter = "";
        }
        else if (scope.departmentIds.empty())
        {
            visibleFilter = " AND 1 = 0 ";
        }
        else
        {
            visibleFilter = " AND u.account_type = 'staff' AND pos.department_id IN (" +
                            joinIds(scope.departmentIds) + ") ";
        }

        mysqlx::Row row = dbManager->getSession()
                              ->sql(std::string(
                                        "SELECT u.id, COALESCE(u.position_id, 0), "
                                        "CASE WHEN u.account_type = 'customer' THEN '普通用户' ELSE COALESCE(pos.name, '') END, "
                                        "u.name, ph.phone, u.email, "
                                        "COALESCE(pos.staff_kind, ''), COALESCE(pos.assignment_policy, ''), "
                                        "COALESCE(d.name, ''), COALESCE(u.account_type, ''), "
                                        "COALESCE(pos.status, ''), COALESCE(d.id, 0) "
                                        "FROM users u "
                                        "LEFT JOIN positions pos ON pos.id = u.position_id "
                                        "LEFT JOIN departments d ON d.id = pos.department_id "
                                        "LEFT JOIN phones ph ON ph.user_id = u.id "
                                        "WHERE u.id = ? ") +
                                    hideHigh + visibleFilter + " LIMIT 1")
                              .bind(employeeId)
                              .execute()
                              .fetchOne();
        if (!row)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        nlohmann::json data = {
            {"id", row[0].get<int>()},
            {"position_id", row[1].isNull() || row[1].get<int>() == 0
                                ? nlohmann::json(nullptr)
                                : nlohmann::json(row[1].get<int>())},
            {"position_name", row[2].isNull() ? "" : row[2].get<std::string>()},
            {"name", row[3].isNull() ? "" : clean_string(row[3].get<std::string>())},
            {"phone", row[4].isNull() ? "" : clean_string(row[4].get<std::string>())},
            {"email", row[5].isNull() ? "" : clean_string(row[5].get<std::string>())},
            {"staff_kind", row[6].isNull() ? "" : row[6].get<std::string>()},
            {"assignment_policy", row[7].isNull() ? "" : row[7].get<std::string>()},
            {"department_name", row[8].isNull() ? "" : row[8].get<std::string>()},
            {"account_type", row[9].isNull() ? "" : row[9].get<std::string>()},
            {"position_status", row[10].isNull() ? "" : row[10].get<std::string>()},
            {"department_id", row[11].isNull() || row[11].get<int>() == 0
                                  ? nlohmann::json(nullptr)
                                  : nlohmann::json(row[11].get<int>())},
        };

        // 权限摘要只读
        if (row[1].get<int>() > 0)
        {
            nlohmann::json summary = nlohmann::json::array();
            for (const auto &key : RbacService::loadPermissionsForPosition(dbManager, row[1].get<int>()))
            {
                summary.push_back(key);
            }
            data["permission_summary"] = summary;
        }
        else
        {
            data["permission_summary"] = nlohmann::json::array();
        }

        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response personnelHandler::listDepartments(const crow::request &req, int operatorUserId)
{
    try
    {
        const std::string scopeFilter = orgScopeConditionForUser(operatorUserId, dbManager, "d.id");
        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT d.id, d.branch_id, COALESCE(b.name, ''), d.name, "
                                             "COALESCE(d.description, ''), COALESCE(d.business_domain, 'general') "
                                             "FROM departments d "
                                             "LEFT JOIN branches b ON b.id = d.branch_id "
                                             "WHERE 1=1 " +
                                             scopeFilter +
                                             " ORDER BY d.branch_id, d.id")
                                       .execute();
        nlohmann::json departments = nlohmann::json::array();
        for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
        {
            departments.push_back({
                {"id", row[0].get<int>()},
                {"branch_id", row[1].isNull() ? 0 : row[1].get<int>()},
                {"branch_name", row[2].isNull() ? "" : row[2].get<std::string>()},
                {"name", row[3].isNull() ? "" : row[3].get<std::string>()},
                {"description", row[4].isNull() ? "" : row[4].get<std::string>()},
                {"business_domain", row[5].isNull() ? "general" : row[5].get<std::string>()},
            });
        }
        return ResponseHelper::success(req, {{"departments", departments}});
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response personnelHandler::listPositions(
    const crow::request &req,
    int operatorUserId,
    const nlohmann::json &query)
{
    try
    {
        const int departmentId = getJsonInt(query, "departmentId", 0);
        const bool assignableOnly = query.contains("assignableOnly") && query["assignableOnly"].is_boolean()
                                        ? query["assignableOnly"].get<bool>()
                                        : false;

        const std::string scopeFilter = orgScopeConditionForUser(operatorUserId, dbManager, "p.department_id");
        std::string filters = " WHERE p.status = 'published' " + scopeFilter;
        if (departmentId > 0)
        {
            filters += " AND p.department_id = " + std::to_string(departmentId) + " ";
        }
        if (assignableOnly)
        {
            // 人事可见可直接派 + 需审批的职位（可发起申请）；隐藏 super_admin_only
            filters += " AND COALESCE(p.assignment_policy, 'super_admin_only') <> 'super_admin_only' ";
            filters += " AND COALESCE(p.system_key, '') <> 'super-admin' ";
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT p.id, p.department_id, COALESCE(d.name, ''), p.name, "
                                             "COALESCE(p.system_key, ''), COALESCE(p.staff_kind, ''), "
                                             "COALESCE(p.description, ''), COALESCE(p.assignment_policy, 'super_admin_only'), "
                                             "COALESCE(p.status, 'draft') "
                                             "FROM positions p "
                                             "LEFT JOIN departments d ON d.id = p.department_id " +
                                             filters +
                                             " ORDER BY p.department_id, p.id")
                                       .execute();

        nlohmann::json positions = nlohmann::json::array();
        for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
        {
            const int positionId = row[0].get<int>();
            nlohmann::json permissionSummary = nlohmann::json::array();
            for (const auto &key : RbacService::loadPermissionsForPosition(dbManager, positionId))
            {
                permissionSummary.push_back(key);
            }
            positions.push_back({
                {"id", positionId},
                {"department_id", row[1].isNull() ? nlohmann::json(nullptr) : nlohmann::json(row[1].get<int>())},
                {"department_name", row[2].isNull() ? "" : row[2].get<std::string>()},
                {"name", row[3].isNull() ? "" : row[3].get<std::string>()},
                {"system_key", row[4].isNull() ? "" : row[4].get<std::string>()},
                {"staff_kind", row[5].isNull() ? "" : row[5].get<std::string>()},
                {"description", row[6].isNull() ? "" : row[6].get<std::string>()},
                {"assignment_policy", row[7].isNull() ? "super_admin_only" : row[7].get<std::string>()},
                {"status", row[8].isNull() ? "draft" : row[8].get<std::string>()},
                {"permission_summary", permissionSummary},
            });
        }
        return ResponseHelper::success(req, {{"positions", positions}});
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response personnelHandler::updateEmployeeAssignment(
    const crow::request &req,
    int operatorUserId,
    int employeeId,
    const nlohmann::json &body)
{
    try
    {
        if (employeeId <= 0)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        EmploymentAssignmentService::AssignRequest assignReq;
        assignReq.operatorUserId = operatorUserId;
        assignReq.targetUserId = employeeId;
        assignReq.mode = EmploymentAssignmentService::ActorMode::Personnel;
        assignReq.reason = getJsonString(body, "reason");
        assignReq.effectiveFrom = getJsonString(body, "effective_from");

        if (!body.contains("expected_current_position_id") || body["expected_current_position_id"].is_null())
        {
            // 允许数字 0 表示客户；null/缺失均拒绝
            return ResponseHelper::validation(req, "expected_current_position_id 必填");
        }
        if (!body["expected_current_position_id"].is_number_integer())
        {
            return ResponseHelper::validation(req, "expected_current_position_id 必须是整数");
        }
        assignReq.expectedCurrentPositionId = body["expected_current_position_id"].get<int>();
        assignReq.hasExpectedCurrentPosition = true;

        // B16: action 必须显式为 onboard/transfer/offboard，禁止空值推断。
        const std::string action = getJsonString(body, "action");
        if (action.empty())
        {
            return ResponseHelper::validation(req, "action 必填");
        }
        if (action == "regularize")
        {
            return ResponseHelper::validation(req, "转正请使用独立 regularization 接口");
        }
        if (action == "offboard")
        {
            assignReq.action = EmploymentAssignmentService::Action::Offboard;
        }
        else if (action == "transfer")
        {
            assignReq.action = EmploymentAssignmentService::Action::Transfer;
            const int positionId = getJsonInt(body, "position_id", 0);
            if (positionId <= 0)
            {
                return ResponseHelper::validation(req, "position_id 不能为空");
            }
            assignReq.targetPositionId = positionId;
        }
        else if (action == "onboard")
        {
            const int positionId = getJsonInt(body, "position_id", 0);
            if (positionId <= 0)
            {
                return ResponseHelper::validation(req, "position_id 不能为空");
            }
            assignReq.targetPositionId = positionId;
            assignReq.action = EmploymentAssignmentService::Action::Onboard;
        }
        else
        {
            return ResponseHelper::validation(req, "action 取值不合法");
        }

        const auto result = EmploymentAssignmentService::assign(dbManager, assignReq);
        return assignmentResultToResponse(req, result);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

namespace
{
// 转正/离职共用：校验 expected_current_position_id + reason，解析可选 break_glass。
bool parseLifecycleRequestBody(
    const crow::request &req,
    const nlohmann::json &body,
    EmploymentAssignmentService::AssignRequest &assignReq,
    crow::response &errorOut)
{
    assignReq.reason = getJsonString(body, "reason");
    if (assignReq.reason.empty())
    {
        errorOut = ResponseHelper::validation(req, "reason 不能为空");
        return false;
    }
    assignReq.effectiveFrom = getJsonString(body, "effective_from");

    if (!body.contains("expected_current_position_id") || body["expected_current_position_id"].is_null())
    {
        errorOut = ResponseHelper::validation(req, "expected_current_position_id 必填");
        return false;
    }
    if (!body["expected_current_position_id"].is_number_integer())
    {
        errorOut = ResponseHelper::validation(req, "expected_current_position_id 必须是整数");
        return false;
    }
    assignReq.expectedCurrentPositionId = body["expected_current_position_id"].get<int>();
    assignReq.hasExpectedCurrentPosition = true;

    // 显式 break-glass：action 必须为 "break_glass"；缺省/未知 fail closed 为普通审批流。
    if (body.contains("action") && !body["action"].is_null())
    {
        if (!body["action"].is_string())
        {
            errorOut = ResponseHelper::validation(req, "action 取值不合法");
            return false;
        }
        const std::string actionStr = body["action"].get<std::string>();
        if (actionStr == "break_glass")
        {
            assignReq.explicitBreakGlass = true;
        }
        else if (!actionStr.empty())
        {
            errorOut = ResponseHelper::validation(req, "action 取值不合法");
            return false;
        }
    }
    return true;
}
}

crow::response personnelHandler::createRegularization(
    const crow::request &req,
    int operatorUserId,
    int employeeId,
    const nlohmann::json &body)
{
    try
    {
        if (employeeId <= 0)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        EmploymentAssignmentService::AssignRequest assignReq;
        assignReq.operatorUserId = operatorUserId;
        assignReq.targetUserId = employeeId;
        assignReq.mode = EmploymentAssignmentService::ActorMode::Personnel;
        assignReq.action = EmploymentAssignmentService::Action::Regularize;

        crow::response err;
        if (!parseLifecycleRequestBody(req, body, assignReq, err))
        {
            return err;
        }

        const auto result = EmploymentAssignmentService::assign(dbManager, assignReq);
        return assignmentResultToResponse(req, result);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response personnelHandler::createOffboarding(
    const crow::request &req,
    int operatorUserId,
    int employeeId,
    const nlohmann::json &body)
{
    try
    {
        if (employeeId <= 0)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        EmploymentAssignmentService::AssignRequest assignReq;
        assignReq.operatorUserId = operatorUserId;
        assignReq.targetUserId = employeeId;
        assignReq.mode = EmploymentAssignmentService::ActorMode::Personnel;
        assignReq.action = EmploymentAssignmentService::Action::Offboard;

        crow::response err;
        if (!parseLifecycleRequestBody(req, body, assignReq, err))
        {
            return err;
        }

        const auto result = EmploymentAssignmentService::assign(dbManager, assignReq);
        return assignmentResultToResponse(req, result);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}
