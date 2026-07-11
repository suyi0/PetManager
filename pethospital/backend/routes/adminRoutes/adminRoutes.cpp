#include "adminRoutes.h"
#include "../../controllers/modules/doctor/doctorHandler.h"
#include "../../controllers/modules/finance/financeHandler.h"
#include "../../controllers/modules/personnel/personnelHandler.h"
#include "../../controllers/modules/reportTemplate/ReportTemplateHandler.h"
#include "../../services/realtime/doctorBroadcaster/doctorQueueBroadcaster.h"
#include "../../services/realtime/doctorListBroadcaster/doctorListBroadcaster.h"
#include "../../services/realtime/financeBroadcaster/financeHomeDataBroadcaster.h"
#include "../../services/realtime/medicineBroadcaster/medicineStockBroadcaster.h"
#include "../../services/logger/operationLogger.h"
#include "../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../services/attendance/DevicePersonSync.h"
#include "../../services/auth/AccessRevocation.h"
#include "../../services/auth/AuthSessionStore.h"
#include "../../services/rbac/RbacService.h"
#include "../../services/redis/userRoleCache/UserRoleCache.h"
#include "../../utils/permissions/Permissions.h"

#include <iostream>
#include <optional>
#include <set>

namespace
{
bool hasUsableDb(const std::shared_ptr<DatabaseManagerInterface> &dbManager)
{
    return dbManager && dbManager->getSession() && dbManager->getSchema();
}

std::optional<int> jsonInt(const nlohmann::json &body, const std::string &key)
{
    if (!body.contains(key) || body[key].is_null())
    {
        return std::nullopt;
    }
    if (!body[key].is_number_integer())
    {
        return std::nullopt;
    }
    return body[key].get<int>();
}

std::string jsonString(const nlohmann::json &body, const std::string &key, const std::string &fallback = "")
{
    if (!body.contains(key) || body[key].is_null())
    {
        return fallback;
    }
    if (!body[key].is_string())
    {
        return fallback;
    }
    return body[key].get<std::string>();
}

nlohmann::json rowToDepartmentJson(const mysqlx::Row &row)
{
    return {
        {"id", row[0].isNull() ? 0 : row[0].get<int>()},
        {"branch_id", row[1].isNull() ? 0 : row[1].get<int>()},
        {"branch_name", row[2].isNull() ? "" : row[2].get<std::string>()},
        {"name", row[3].isNull() ? "" : row[3].get<std::string>()},
        {"description", row[4].isNull() ? "" : row[4].get<std::string>()},
    };
}

nlohmann::json rowToPositionJson(const mysqlx::Row &row)
{
    return {
        {"id", row[0].isNull() ? 0 : row[0].get<int>()},
        {"department_id", row[1].isNull() ? nullptr : nlohmann::json(row[1].get<int>())},
        {"department_name", row[2].isNull() ? "" : row[2].get<std::string>()},
        {"name", row[3].isNull() ? "" : row[3].get<std::string>()},
        {"system_key", row[4].isNull() ? "" : row[4].get<std::string>()},
        {"staff_kind", row[5].isNull() ? "" : row[5].get<std::string>()},
        {"description", row[6].isNull() ? "" : row[6].get<std::string>()},
    };
}

bool positionExists(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int positionId)
{
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT id FROM positions WHERE id = ? LIMIT 1")
                                  .bind(positionId)
                                  .execute();
    return static_cast<bool>(result.fetchOne());
}

bool branchExists(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int branchId)
{
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT id FROM branches WHERE id = ? LIMIT 1")
                                  .bind(branchId)
                                  .execute();
    return static_cast<bool>(result.fetchOne());
}

bool departmentExists(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int departmentId)
{
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT id FROM departments WHERE id = ? LIMIT 1")
                                  .bind(departmentId)
                                  .execute();
    return static_cast<bool>(result.fetchOne());
}

int defaultBranchId(const std::shared_ptr<DatabaseManagerInterface> &dbManager)
{
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT id FROM branches WHERE system_key = 'main' LIMIT 1")
                                  .execute();
    mysqlx::Row row = result.fetchOne();
    return row && !row[0].isNull() ? row[0].get<int>() : 0;
}

bool isSuperAdminPosition(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int positionId)
{
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT id FROM positions WHERE id = ? AND system_key = 'super-admin' LIMIT 1")
                                  .bind(positionId)
                                  .execute();
    return static_cast<bool>(result.fetchOne());
}

std::optional<std::vector<std::string>> parseGrantablePermissions(const crow::request &req, const nlohmann::json &body, crow::response &res)
{
    std::vector<std::string> permissions;
    std::set<std::string> uniquePermissions;
    if (!body.contains("permissions") || !body["permissions"].is_array())
    {
        res = ResponseHelper::validation(req, "permissions 必须是数组");
        return std::nullopt;
    }

    for (const auto &item : body["permissions"])
    {
        if (!item.is_string())
        {
            res = ResponseHelper::validation(req, "permissions 只能包含字符串");
            return std::nullopt;
        }

        const std::string permissionKey = item.get<std::string>();
        if (permissionKey == Permissions::kRbacManage || !Permissions::isGrantablePermissionKey(permissionKey))
        {
            res = ResponseHelper::permission_denied(req, "权限不允许授权");
            return std::nullopt;
        }

        if (uniquePermissions.insert(permissionKey).second)
        {
            permissions.push_back(permissionKey);
        }
    }

    return permissions;
}

std::vector<std::string> loadTemplatePermissions(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int templateId)
{
    std::vector<std::string> permissions;
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT permission_key FROM permission_template_items WHERE template_id = ?")
                                  .bind(templateId)
                                  .execute();

    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
    {
        if (!row[0].isNull())
        {
            const std::string permissionKey = row[0].get<std::string>();
            if (permissionKey != Permissions::kRbacManage && Permissions::isGrantablePermissionKey(permissionKey))
            {
                permissions.push_back(permissionKey);
            }
        }
    }
    return permissions;
}

std::optional<std::set<int>> parseIntSet(const crow::request &req, const nlohmann::json &body, const std::string &key, crow::response &res)
{
    std::set<int> values;
    if (!body.contains(key) || body[key].is_null())
    {
        return values;
    }
    if (!body[key].is_array())
    {
        res = ResponseHelper::validation(req, key + " 必须是数组");
        return std::nullopt;
    }

    for (const auto &item : body[key])
    {
        if (!item.is_number_integer())
        {
            res = ResponseHelper::validation(req, key + " 只能包含整数");
            return std::nullopt;
        }
        const int value = item.get<int>();
        if (value <= 0)
        {
            res = ResponseHelper::validation(req, key + " 只能包含正整数");
            return std::nullopt;
        }
        values.insert(value);
    }
    return values;
}

bool replacePositionPermissions(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId,
    const std::vector<std::string> &permissions)
{
    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        session->sql("DELETE FROM position_permissions WHERE position_id = ?")
            .bind(positionId)
            .execute();

        for (const std::string &permissionKey : permissions)
        {
            session->sql("INSERT INTO position_permissions (position_id, permission_key) VALUES (?, ?)")
                .bind(positionId, permissionKey)
                .execute();
        }

        session->sql("COMMIT").execute();
        return true;
    }
    catch (...)
    {
        rollbackTransactionQuietly(*session);
        throw;
    }
}

void bumpUsersInPosition(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int positionId)
{
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT id FROM users WHERE position_id = ? AND is_deleted = 0")
                                  .bind(positionId)
                                  .execute();

    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
    {
        if (!row[0].isNull())
        {
            const int userId = row[0].get<int>();
            AccessRevocation::revokeUserSessions(userId);
        }
    }
}

// 断实时连接的实现统一在 services/auth/AccessRevocation（admin 与 personnel 派职共用，防两处漂移）

crow::response getDepartments(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager)
{
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT d.id, d.branch_id, COALESCE(b.name, ''), d.name, COALESCE(d.description, '') "
                                        "FROM departments AS d "
                                        "LEFT JOIN branches AS b ON b.id = d.branch_id "
                                        "ORDER BY d.branch_id, d.id")
                                  .execute();
    nlohmann::json departments = nlohmann::json::array();
    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
    {
        departments.push_back(rowToDepartmentJson(row));
    }
    return ResponseHelper::success(req, {{"departments", departments}});
}

crow::response createDepartment(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, const nlohmann::json &body)
{
    const std::string name = jsonString(body, "name");
    const std::string description = jsonString(body, "description");
    if (name.empty())
    {
        return ResponseHelper::validation(req, "部门名称不能为空");
    }
    const int branchId = jsonInt(body, "branch_id").value_or(defaultBranchId(dbManager));
    if (branchId <= 0 || !branchExists(dbManager, branchId))
    {
        return ResponseHelper::validation(req, "分院不存在");
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("INSERT INTO departments (branch_id, name, description) VALUES (?, ?, ?)")
                                  .bind(branchId, name, description)
                                  .execute();

    return ResponseHelper::created(req, {{"id", static_cast<int>(result.getAutoIncrementValue())}, {"branch_id", branchId}, {"name", name}, {"description", description}});
}

crow::response getPositions(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager)
{
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT p.id, p.department_id, COALESCE(d.name, ''), p.name, "
                                        "COALESCE(p.system_key, ''), COALESCE(p.staff_kind, ''), COALESCE(p.description, '') "
                                        "FROM positions AS p "
                                        "LEFT JOIN departments AS d ON d.id = p.department_id "
                                        "ORDER BY COALESCE(p.department_id, 0), p.id")
                                  .execute();
    nlohmann::json positions = nlohmann::json::array();
    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
    {
        positions.push_back(rowToPositionJson(row));
    }
    return ResponseHelper::success(req, {{"positions", positions}});
}

crow::response createPosition(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, const nlohmann::json &body)
{
    const std::string name = jsonString(body, "name");
    if (name.empty())
    {
        return ResponseHelper::validation(req, "岗位名称不能为空");
    }

    const std::optional<int> departmentId = jsonInt(body, "department_id");
    if (!departmentId.has_value() || !departmentExists(dbManager, departmentId.value()))
    {
        return ResponseHelper::validation(req, "部门不存在");
    }
    const std::string staffKind = jsonString(body, "staff_kind", "general_staff");
    // staff_kind 必须是已知枚举，否则 ENUM 列会抛 500；这里给干净的 400
    static const std::set<std::string> kStaffKinds = {
        "doctor", "nurse", "warehouse", "finance", "management", "personnel", "general_staff"};
    if (kStaffKinds.find(staffKind) == kStaffKinds.end())
    {
        return ResponseHelper::validation(req, "staff_kind 取值不合法");
    }
    const std::string description = jsonString(body, "description");

    // system_key 是系统/seed 专用锚点（受保护职位、业务身份 findPositionIdBySystemKey 用），
    // 用户建的职位一律 NULL：既避免多个 '' 撞 UNIQUE，也禁止用户占用 'doctor' 之类锚点。
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("INSERT INTO positions (department_id, name, system_key, staff_kind, description) VALUES (?, ?, NULL, ?, ?)")
                                  .bind(departmentId.value(), name, staffKind, description)
                                  .execute();

    return ResponseHelper::created(req, {{"id", static_cast<int>(result.getAutoIncrementValue())}, {"name", name}});
}

crow::response getPositionPermissions(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, int positionId)
{
    if (!positionExists(dbManager, positionId))
    {
        return ResponseHelper::notFound(req, "岗位不存在");
    }

    nlohmann::json permissions = nlohmann::json::array();
    for (const std::string &permissionKey : RbacService::loadPermissionsForPosition(dbManager, positionId))
    {
        if (permissionKey != Permissions::kRbacManage)
        {
            permissions.push_back(permissionKey);
        }
    }
    return ResponseHelper::success(req, {{"position_id", positionId}, {"permissions", permissions}});
}

crow::response getPermissionTemplates(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager)
{
    nlohmann::json templates = nlohmann::json::array();
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT id, name FROM permission_templates ORDER BY id")
                                  .execute();
    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
    {
        templates.push_back({
            {"id", row[0].isNull() ? 0 : row[0].get<int>()},
            {"name", row[1].isNull() ? "" : row[1].get<std::string>()},
        });
    }
    return ResponseHelper::success(req, {{"templates", templates}});
}

crow::response updatePositionPermissions(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, int positionId, const nlohmann::json &body)
{
    if (!positionExists(dbManager, positionId))
    {
        return ResponseHelper::notFound(req, "岗位不存在");
    }
    if (isSuperAdminPosition(dbManager, positionId))
    {
        return ResponseHelper::permission_denied(req, "系统超级管理员岗位权限不允许修改");
    }

    crow::response validationRes;
    std::optional<std::vector<std::string>> permissions = parseGrantablePermissions(req, body, validationRes);
    if (!permissions.has_value())
    {
        return validationRes;
    }

    replacePositionPermissions(dbManager, positionId, permissions.value());
    bumpUsersInPosition(dbManager, positionId);
    AccessRevocation::closeRealtimeConnections();
    return ResponseHelper::success(req, {{"position_id", positionId}, {"permissions", permissions.value()}});
}

crow::response applyPermissionTemplate(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, int positionId, const nlohmann::json &body)
{
    if (!positionExists(dbManager, positionId))
    {
        return ResponseHelper::notFound(req, "岗位不存在");
    }
    if (isSuperAdminPosition(dbManager, positionId))
    {
        return ResponseHelper::permission_denied(req, "系统超级管理员岗位权限不允许修改");
    }

    const std::optional<int> templateId = jsonInt(body, "template_id");
    if (!templateId.has_value())
    {
        return ResponseHelper::validation(req, "template_id 必须是整数");
    }

    std::vector<std::string> permissions = loadTemplatePermissions(dbManager, templateId.value());
    if (permissions.empty())
    {
        return ResponseHelper::notFound(req, "权限模板不存在或为空");
    }

    replacePositionPermissions(dbManager, positionId, permissions);
    bumpUsersInPosition(dbManager, positionId);
    AccessRevocation::closeRealtimeConnections();
    return ResponseHelper::success(req, {{"position_id", positionId}, {"template_id", templateId.value()}, {"permissions", permissions}});
}

crow::response updateUserPosition(
    const crow::request &req,
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId,
    int targetUserId,
    const nlohmann::json &body)
{
    if (targetUserId <= 0)
    {
        return ResponseHelper::validation(req, "用户ID无效");
    }

    const std::optional<int> positionId = jsonInt(body, "position_id");
    const bool assignCustomer = !positionId.has_value() || positionId.value() <= 0;
    bool targetPositionHasPermissions = false;
    if (!assignCustomer)
    {
        if (!positionExists(dbManager, positionId.value()))
        {
            return ResponseHelper::notFound(req, "岗位不存在");
        }
        if (isSuperAdminPosition(dbManager, positionId.value()))
        {
            return ResponseHelper::permission_denied(req, "不能通过接口授予系统超级管理员岗位");
        }
        targetPositionHasPermissions = !RbacService::loadPermissionsForPosition(dbManager, positionId.value()).empty();
        if (targetPositionHasPermissions &&
            !RbacService::userHasPermission(dbManager, operatorUserId, Permissions::kRbacManage))
        {
            return ResponseHelper::permission_denied(req, "分配带权限的岗位需要权限管理权限");
        }
        if (operatorUserId == targetUserId && targetPositionHasPermissions)
        {
            return ResponseHelper::permission_denied(req, "不能给自己改派到带权限的岗位");
        }
    }

    mysqlx::SqlResult userResult = dbManager->getSession()
                                      ->sql("SELECT id FROM users WHERE id = ? AND is_deleted = 0 LIMIT 1")
                                      .bind(targetUserId)
                                      .execute();
    if (!userResult.fetchOne())
    {
        return ResponseHelper::notFound(req, "用户不存在");
    }

    if (assignCustomer)
    {
        dbManager->getSession()
            ->sql("UPDATE users SET account_type = 'customer', position_id = NULL WHERE id = ?")
            .bind(targetUserId)
            .execute();
    }
    else
    {
        dbManager->getSession()
            ->sql("UPDATE users SET account_type = 'staff', position_id = ? WHERE id = ?")
            .bind(positionId.value(), targetUserId)
            .execute();
    }

    AccessRevocation::revokeUserSessions(targetUserId);
    AccessRevocation::closeRealtimeConnections();
    // 考勤设备联动：转客户=删设备模板（撤权第四件套），派职=下发模板（内部会初始化 attendance_no）。
    if (assignCustomer)
    {
        DevicePersonSync::enqueueRemove(dbManager, targetUserId);
    }
    else
    {
        DevicePersonSync::enqueueUpsert(dbManager, targetUserId);
    }

    return ResponseHelper::success(req, {{"user_id", targetUserId}, {"position_id", assignCustomer ? nullptr : nlohmann::json(positionId.value())}});
}

crow::response getUserScopes(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, int targetUserId)
{
    mysqlx::SqlResult userResult = dbManager->getSession()
                                      ->sql("SELECT id FROM users WHERE id = ? AND is_deleted = 0 LIMIT 1")
                                      .bind(targetUserId)
                                      .execute();
    if (!userResult.fetchOne())
    {
        return ResponseHelper::notFound(req, "用户不存在");
    }

    nlohmann::json branchIds = nlohmann::json::array();
    nlohmann::json departmentIds = nlohmann::json::array();
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT branch_id, department_id FROM user_scopes WHERE user_id = ? ORDER BY id")
                                  .bind(targetUserId)
                                  .execute();
    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
    {
        if (!row[0].isNull())
        {
            branchIds.push_back(row[0].get<int>());
        }
        if (!row[1].isNull())
        {
            departmentIds.push_back(row[1].get<int>());
        }
    }

    return ResponseHelper::success(req, {{"user_id", targetUserId}, {"branch_ids", branchIds}, {"department_ids", departmentIds}});
}

crow::response replaceUserScopes(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId,
    int targetUserId,
    const crow::request &req,
    const nlohmann::json &body)
{
    mysqlx::SqlResult userResult = dbManager->getSession()
                                      ->sql("SELECT id FROM users WHERE id = ? AND is_deleted = 0 LIMIT 1")
                                      .bind(targetUserId)
                                      .execute();
    if (!userResult.fetchOne())
    {
        return ResponseHelper::notFound(req, "用户不存在");
    }

    crow::response validationRes;
    std::optional<std::set<int>> branchIds = parseIntSet(req, body, "branch_ids", validationRes);
    if (!branchIds.has_value())
    {
        return validationRes;
    }
    std::optional<std::set<int>> departmentIds = parseIntSet(req, body, "department_ids", validationRes);
    if (!departmentIds.has_value())
    {
        return validationRes;
    }

    for (const int branchId : branchIds.value())
    {
        if (!branchExists(dbManager, branchId))
        {
            return ResponseHelper::validation(req, "分院范围不存在");
        }
    }
    for (const int departmentId : departmentIds.value())
    {
        if (!departmentExists(dbManager, departmentId))
        {
            return ResponseHelper::validation(req, "部门范围不存在");
        }
    }

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        session->sql("DELETE FROM user_scopes WHERE user_id = ?")
            .bind(targetUserId)
            .execute();

        for (const int branchId : branchIds.value())
        {
            session->sql("INSERT INTO user_scopes (user_id, branch_id, department_id, granted_by) VALUES (?, ?, NULL, ?)")
                .bind(targetUserId, branchId, userId)
                .execute();
        }
        for (const int departmentId : departmentIds.value())
        {
            session->sql("INSERT INTO user_scopes (user_id, branch_id, department_id, granted_by) VALUES (?, NULL, ?, ?)")
                .bind(targetUserId, departmentId, userId)
                .execute();
        }

        session->sql("COMMIT").execute();
    }
    catch (...)
    {
        rollbackTransactionQuietly(*session);
        throw;
    }

    AccessRevocation::revokeUserSessions(targetUserId);
    AccessRevocation::closeRealtimeConnections();

    return getUserScopes(req, dbManager, targetUserId);
}
}

void adminRoutes::setupAdminRoutes(
    CrowApp &app,
    std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }

    CROW_ROUTE(app, "/api/admin/org/departments")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                const bool isCreate = req.method == crow::HTTPMethod::Post;
                const std::string action = isCreate ? "创建部门" : "获取部门列表";
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action);
                        return;
                    }
                    if (!hasUsableDb(dbManager))
                    {
                        res = ResponseHelper::system_error(req, "Database connection unavailable");
                        return;
                    }

                    if (isCreate)
                    {
                        BaseHandler parser(dbManager);
                        auto jsonOpt = parser.parseJson(req, res);
                        if (!jsonOpt)
                        {
                            OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt);
                            return;
                        }
                        res = createDepartment(req, dbManager, jsonOpt.value());
                    }
                    else
                    {
                        res = getDepartments(req, dbManager);
                    }
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt, isCreate);
            });

    CROW_ROUTE(app, "/api/admin/org/positions")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                const bool isCreate = req.method == crow::HTTPMethod::Post;
                const std::string action = isCreate ? "创建岗位" : "获取岗位列表";
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action);
                        return;
                    }
                    if (!hasUsableDb(dbManager))
                    {
                        res = ResponseHelper::system_error(req, "Database connection unavailable");
                        return;
                    }

                    if (isCreate)
                    {
                        BaseHandler parser(dbManager);
                        auto jsonOpt = parser.parseJson(req, res);
                        if (!jsonOpt)
                        {
                            OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt);
                            return;
                        }
                        res = createPosition(req, dbManager, jsonOpt.value());
                    }
                    else
                    {
                        res = getPositions(req, dbManager);
                    }
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt, isCreate);
            });

    CROW_ROUTE(app, "/api/admin/rbac/positions/<int>/permissions")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Put, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int positionId)
            {
                int userId = -1;
                const bool isUpdate = req.method == crow::HTTPMethod::Put;
                const std::string action = isUpdate ? "修改岗位权限" : "获取岗位权限";
                try
                {
                    // 授权/收权(PUT)=改动实权，必须 rbac:manage(超管独占，与 apply-template 同级)；
                    // 只读(GET)查看某岗位有哪些权限，管理端可见即可。
                    // 若两者都只用 isValidManagementToken，任一管理用户可给自己岗位授
                    // portal:super-admin/scope:all 等，绕过"授权归超管"边界(Codex 2026-07-08 抓出的回退)。
                    userId = isUpdate
                                 ? isValidPermissionToken(req, res, dbManager, Permissions::kRbacManage)
                                 : isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action);
                        return;
                    }
                    if (!hasUsableDb(dbManager))
                    {
                        res = ResponseHelper::system_error(req, "Database connection unavailable");
                        return;
                    }

                    if (isUpdate)
                    {
                        BaseHandler parser(dbManager);
                        auto jsonOpt = parser.parseJson(req, res);
                        if (!jsonOpt)
                        {
                            OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt);
                            return;
                        }
                        res = updatePositionPermissions(req, dbManager, positionId, jsonOpt.value());
                    }
                    else
                    {
                        res = getPositionPermissions(req, dbManager, positionId);
                    }
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", action, Permissions::kRbacManage, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admin/rbac/positions/<int>/apply-template")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int positionId)
            {
                int userId = -1;
                const std::string action = "应用岗位权限模板";
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kRbacManage);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action);
                        return;
                    }
                    if (!hasUsableDb(dbManager))
                    {
                        res = ResponseHelper::system_error(req, "Database connection unavailable");
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto jsonOpt = parser.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }
                    res = applyPermissionTemplate(req, dbManager, positionId, jsonOpt.value());
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", action, Permissions::kRbacManage, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admin/rbac/permission-templates")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                const std::string action = "获取权限模板";
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kRbacManage);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action);
                        return;
                    }
                    if (!hasUsableDb(dbManager))
                    {
                        res = ResponseHelper::system_error(req, "Database connection unavailable");
                        return;
                    }

                    res = getPermissionTemplates(req, dbManager);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", action, Permissions::kRbacManage, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admin/users/<int>/position")
        .methods(crow::HTTPMethod::Put, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int targetUserId)
            {
                int userId = -1;
                const std::string action = "修改用户岗位";
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action);
                        return;
                    }
                    if (!hasUsableDb(dbManager))
                    {
                        res = ResponseHelper::system_error(req, "Database connection unavailable");
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto jsonOpt = parser.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }
                    res = updateUserPosition(req, dbManager, userId, targetUserId, jsonOpt.value());
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", action, Permissions::kRbacManage, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admin/users/<int>/scopes")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Put, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int targetUserId)
            {
                int userId = -1;
                const bool isUpdate = req.method == crow::HTTPMethod::Put;
                const std::string action = isUpdate ? "修改用户组织范围" : "获取用户组织范围";
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kRbacManage);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action);
                        return;
                    }
                    if (!hasUsableDb(dbManager))
                    {
                        res = ResponseHelper::system_error(req, "Database connection unavailable");
                        return;
                    }

                    if (isUpdate)
                    {
                        BaseHandler parser(dbManager);
                        auto jsonOpt = parser.parseJson(req, res);
                        if (!jsonOpt)
                        {
                            OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt);
                            return;
                        }
                        res = replaceUserScopes(dbManager, userId, targetUserId, req, jsonOpt.value());
                    }
                    else
                    {
                        res = getUserScopes(req, dbManager, targetUserId);
                    }
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", action, Permissions::kRbacManage, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admin/rbac/permissions/catalog")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kRbacManage);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "获取权限目录");
                        return;
                    }

                    nlohmann::json permissionKeys = nlohmann::json::array();
                    for (const std::string &permissionKey : Permissions::grantablePermissionKeys())
                    {
                        permissionKeys.push_back(permissionKey);
                    }

                    res = ResponseHelper::success(req, {{"permissions", permissionKeys}});
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "获取权限目录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "获取权限目录", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    // 用于页面首次加载、手动刷新、SSE 断线后的兜底请求。
    CROW_ROUTE(app, "/api/admins/home-data")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidSuperAdminPortalToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "获取超级管理员首页数据");
                        return;
                    }

                    adminHandler handler(dbManager);
                    crow::response response = handler.getHomeData(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "获取超级管理员首页数据", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "获取首页数据", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    // 超级管理员首页实时数据通道。
    CROW_WEBSOCKET_ROUTE(app, "/realtime/admins/home-data")
        .onaccept([dbManager](const crow::request &req, void **userdata)
                  {
            // 前端请求传递的Token值会通过Crow框架自动解析存入req.url_params字典中。
            // req.url_params.get() 返回的是一个 C 风格字符串（const char*）
            const char *tokenParam = req.url_params.get("token");
            if (tokenParam == nullptr || std::string(tokenParam).empty())
            {
                return false;
            }

            auto claims = JwtUtils::getTokenClaims(tokenParam);
            if (!claims || claims->userId <= 0 || !dbManager || !dbManager->getSession())
            {
                return false;
            }

            // 管理端会话失效：被 bump 过的旧 token（改密码/失效后）不能继续建立实时连接。
            if (!AuthSessionStore::isSessionCurrent(claims->userId, claims->sessionVersion))
            {
                return false;
            }

            std::string identifier = claims->identifier;
            if (!JwtUtils::isUserAuthorizedForSuperAdminPortal(
                    claims->userId,
                    identifier,
                    claims->isEmailLogin,
                    dbManager))
            {
                return false;
            }

            auto *context = new AdminHomeDataBroadcaster::ConnectionContext{
                claims->userId,
                claims->sessionVersion};
            *userdata = context;
            return true; })
        .onopen([](crow::websocket::connection &conn)
                {
            auto *context = static_cast<AdminHomeDataBroadcaster::ConnectionContext *>(conn.userdata());
            if (!context || context->userId <= 0)
            {
                conn.close("invalid_admin_home_session");
                return;
            }

            AdminHomeDataBroadcaster::instance().addConnection(&conn, *context);
            delete context;
            conn.userdata(nullptr); })
        .onclose([](crow::websocket::connection &conn, const std::string &, uint16_t)
                 {
            AdminHomeDataBroadcaster::instance().removeConnection(&conn); })
        .onerror([](crow::websocket::connection &conn, const std::string &reason)
                 {
            std::cerr << "Admin homeData WebSocket error: " << reason << std::endl;
            AdminHomeDataBroadcaster::instance().removeConnection(&conn); });

    CROW_ROUTE(app, "/api/admins/users")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                const bool isCreate = req.method == crow::HTTPMethod::Post;
                const std::string action = isCreate ? "创建用户" : "获取用户列表";
                try
                {
                    userId = isValidSuperAdminPortalToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action);
                        return;
                    }

                    crow::response response;
                    if (isCreate)
                    {
                        personnelHandler handler(dbManager);
                        response = handler.createUser(req);
                    }
                    else
                    {
                        adminHandler handler(dbManager);
                        response = handler.getUsers(req);
                    }
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt, isCreate);
            });

    CROW_ROUTE(app, "/api/admins/users/search")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidSuperAdminPortalToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "搜索用户");
                        return;
                    }

                    adminHandler handler(dbManager);
                    auto jsonOpt = handler.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "搜索用户", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    crow::response response = handler.searchUsers(req, jsonOpt.value());
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "搜索用户", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "搜索用户", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    CROW_ROUTE(app, "/api/admins/online-doctors/search")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidSuperAdminPortalToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "搜索在线医生");
                        return;
                    }

                    adminHandler handler(dbManager);
                    auto jsonOpt = handler.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "搜索在线医生", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    crow::response response = handler.searchOnlineDoctors(req, jsonOpt.value());
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "搜索在线医生", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "搜索在线医生", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    CROW_ROUTE(app, "/api/admins/work-time-records")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidSuperAdminPortalToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "获取工时记录");
                        return;
                    }

                    adminHandler handler(dbManager);
                    crow::response response = handler.getWorkTimeRecord(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "获取工时记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "获取工时记录", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    CROW_ROUTE(app, "/api/admins/user-deletions")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kUserDelete);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "删除用户");
                        return;
                    }

                    personnelHandler handler(dbManager);
                    crow::response response = handler.deleteUser(req, userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "删除用户", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "删除用户", Permissions::kUserDelete, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admins/doctor-work-time-changes")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kDoctorWorkWrite);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "调整医生排班");
                        return;
                    }

                    adminHandler handler(dbManager);
                    auto jsonOpt = handler.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "调整医生排班", Permissions::kDoctorWorkWrite, userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    nlohmann::json &request_body = jsonOpt.value();
                    std::string date = request_body["date"].is_string()
                                           ? request_body["date"].get<std::string>()
                                           : request_body.dump();
                    std::string identifier = request_body["identifier"].is_string()
                                                 ? request_body["identifier"].get<std::string>()
                                                 : request_body.dump();

                    crow::response response =
                        handler.changeDoctorWorkTime(req, userId, date, identifier);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "调整医生排班", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "调整医生排班", Permissions::kDoctorWorkWrite, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    // 修改医生工作状态接口
    CROW_ROUTE(app, "/api/admins/doctor-work-status-changes")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kDoctorWorkWrite);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "修改医生工作状态");
                        return;
                    }

                    adminHandler handler(dbManager);
                    crow::response response = handler.handleDoctorStatusAction(req, userId, true);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "修改医生工作状态", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "修改医生工作状态", Permissions::kDoctorWorkWrite, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admins/logs")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kLogsRead);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "查询操作日志");
                        return;
                    }

                    adminHandler handler(dbManager);
                    crow::response response = handler.getLogs(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "查询操作日志", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "查询操作日志", Permissions::kLogsRead, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admins/logs/search")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kLogsRead);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "搜索操作日志");
                        return;
                    }

                    adminHandler handler(dbManager);
                    auto jsonOpt = handler.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "搜索操作日志", Permissions::kLogsRead, userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    crow::response response = handler.searchLogs(req, jsonOpt.value());
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "搜索操作日志", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "搜索操作日志", Permissions::kLogsRead, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admins/order-records")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            int userId = -1;
            try
            {
                userId = isValidPermissionToken(req, res, dbManager, Permissions::kMedicalRecordRead);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "订单", "获取全部病历");
                    return;
                }

                adminHandler handler(dbManager);
                crow::response response = handler.getAllRecord(req, userId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "订单", "获取全部病历", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishSensitiveRoute(dbManager, req, res, "订单", "获取全部病历", Permissions::kMedicalRecordRead, userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    CROW_ROUTE(app, "/api/admin/report-templates/data-contracts/medical-document")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
        {
            const int userId = isValidPermissionToken(req, res, dbManager, Permissions::kReportTemplateRead);
            if (res.code != 200 || userId <= 0)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "读取诊疗单字段目录");
                return;
            }
            ReportTemplateHandler handler(dbManager);
            auto response = handler.dataContract(req);
            ProcessHandlerResponse(req, res, response);
            OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "读取诊疗单字段目录", Permissions::kReportTemplateRead, userId);
        });

    CROW_ROUTE(app, "/api/admin/report-templates")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
        {
            const int userId = isValidPermissionToken(req, res, dbManager, Permissions::kReportTemplateRead);
            if (res.code != 200 || userId <= 0)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "读取打印模板");
                return;
            }
            ReportTemplateHandler handler(dbManager);
            auto response = handler.list(req);
            ProcessHandlerResponse(req, res, response);
            OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "读取打印模板", Permissions::kReportTemplateRead, userId);
        });

    CROW_ROUTE(app, "/api/admin/report-templates/<int>/versions")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int templateId)
        {
            const bool isCreate = req.method == crow::HTTPMethod::Post;
            const char *permission = isCreate ? Permissions::kReportTemplateManage : Permissions::kReportTemplateRead;
            const int userId = isValidPermissionToken(req, res, dbManager, permission);
            if (res.code != 200 || userId <= 0)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", isCreate ? "创建打印模板版本" : "读取打印模板版本");
                return;
            }
            ReportTemplateHandler handler(dbManager);
            auto response = isCreate ? handler.createVersion(req, templateId, userId) : handler.versions(req, templateId);
            ProcessHandlerResponse(req, res, response);
            OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", isCreate ? "创建打印模板版本" : "读取打印模板版本", permission, userId);
        });

    CROW_ROUTE(app, "/api/admin/report-templates/<int>/publish")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int templateId)
        {
            const int userId = isValidPermissionToken(req, res, dbManager, Permissions::kReportTemplatePublish);
            if (res.code != 200 || userId <= 0)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "发布打印模板");
                return;
            }
            ReportTemplateHandler handler(dbManager);
            auto response = handler.publish(req, templateId, userId);
            ProcessHandlerResponse(req, res, response);
            OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "发布打印模板", Permissions::kReportTemplatePublish, userId);
        });

    routes_setup = true;
}
