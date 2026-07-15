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
#include "../../services/employment/EmploymentAssignmentService.h"
#include "../../services/rbac/PositionPermissionService.h"
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
        {"business_domain", row[5].isNull() ? "general" : row[5].get<std::string>()},
        {"is_system", !row[6].isNull() && row[6].get<int>() != 0},
    };
}

// 部门业务域 → 该部门下允许创建的职位工种。普通员工在任何部门可选；
// 各业务域再叠加自己的专属工种。用于 createPosition 强校验（前端引导之外的兜底）。
inline const std::set<std::string> &staffKindsForDomain(const std::string &domain)
{
    static const std::set<std::string> general = {"general_staff"};
    static const std::set<std::string> management = {"general_staff", "management"};
    static const std::set<std::string> finance = {"general_staff", "finance"};
    static const std::set<std::string> personnel = {"general_staff", "personnel"};
    static const std::set<std::string> medical = {"general_staff", "doctor", "nurse"};
    static const std::set<std::string> warehouse = {"general_staff", "warehouse"};
    if (domain == "management") return management;
    if (domain == "finance") return finance;
    if (domain == "personnel") return personnel;
    if (domain == "medical") return medical;
    if (domain == "warehouse") return warehouse;
    return general;
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
        {"assignment_policy", row[7].isNull() ? "super_admin_only" : row[7].get<std::string>()},
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

std::vector<std::string> permissionsOutsideAllowedDomains(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager, int positionId,
    const std::vector<std::string> &permissions)
{
    const auto allowed = RbacService::allowedDomainsForPosition(dbManager, positionId);
    std::vector<std::string> rejected;
    for (const auto &key : permissions)
        if (allowed.count(Permissions::domainOfPermission(key)) == 0) rejected.push_back(key);
    return rejected;
}

crow::response domainBoundaryError(const crow::request &req, const std::vector<std::string> &rejected)
{
    std::string message = "权限超出目标职位允许域：";
    for (std::size_t i = 0; i < rejected.size(); ++i)
    {
        if (i) message += "，";
        message += rejected[i] + "（" + Permissions::domainChineseName(Permissions::domainOfPermission(rejected[i])) + "域）";
    }
    // 不用 422：Crow 1.2.x 状态码表没有它，未知码会被回落成 500。
    // 400 + VALIDATION_ERROR + 明确中文域名信息足够前端与调用方判别。
    return ResponseHelper::validation(req, message);
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

// 职位权限替换统一走 PositionPermissionService（事务内锁行 + 安全下限 + policy 抬升）。

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
                                  ->sql("SELECT d.id, d.branch_id, COALESCE(b.name, ''), d.name, COALESCE(d.description, ''), "
                                        "COALESCE(d.business_domain, 'general'), COALESCE(d.is_system, 0) "
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
    // 业务域：决定该部门下能创建哪些职位工种。缺省 general（仅普通员工）。
    const std::string businessDomain = jsonString(body, "business_domain", "general");
    static const std::set<std::string> kDomains = {
        "general", "management", "finance", "personnel", "medical", "warehouse"};
    if (kDomains.find(businessDomain) == kDomains.end())
    {
        return ResponseHelper::validation(req, "business_domain 取值不合法");
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("INSERT INTO departments (branch_id, name, description, business_domain) VALUES (?, ?, ?, ?)")
                                  .bind(branchId, name, description, businessDomain)
                                  .execute();

    return ResponseHelper::created(req, {{"id", static_cast<int>(result.getAutoIncrementValue())}, {"branch_id", branchId}, {"name", name}, {"description", description}, {"business_domain", businessDomain}});
}

crow::response updateDepartment(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, int departmentId, const nlohmann::json &body)
{
    mysqlx::Row current = dbManager->getSession()
                              ->sql("SELECT name, COALESCE(business_domain, 'general'), COALESCE(is_system, 0) "
                                    "FROM departments WHERE id = ? LIMIT 1")
                              .bind(departmentId)
                              .execute()
                              .fetchOne();
    if (!current)
    {
        return ResponseHelper::notFound(req, "部门不存在");
    }
    // 系统种子部门（管理部等 5 个）是权限边界与业务身份的锚点，名称与业务域一律锁定。
    if (current[2].get<int>() != 0)
    {
        return ResponseHelper::validation(req, "系统内置部门不可编辑");
    }

    const std::string name = jsonString(body, "name", current[0].get<std::string>());
    if (name.empty())
    {
        return ResponseHelper::validation(req, "部门名称不能为空");
    }
    const std::string businessDomain = jsonString(body, "business_domain", current[1].get<std::string>());
    static const std::set<std::string> kDomains = {
        "general", "management", "finance", "personnel", "medical", "warehouse"};
    if (kDomains.find(businessDomain) == kDomains.end())
    {
        return ResponseHelper::validation(req, "business_domain 取值不合法");
    }

    // 改域必须保证已有职位全部落在新域内（与建职位强校验同一条纪律），否则列出冲突职位让管理员先处理。
    if (businessDomain != current[1].get<std::string>())
    {
        const std::set<std::string> &allowed = staffKindsForDomain(businessDomain);
        std::string conflicts;
        mysqlx::SqlResult rows = dbManager->getSession()
                                     ->sql("SELECT name, staff_kind FROM positions WHERE department_id = ?")
                                     .bind(departmentId)
                                     .execute();
        for (mysqlx::Row row = rows.fetchOne(); row; row = rows.fetchOne())
        {
            const std::string kind = row[1].isNull() ? "general_staff" : row[1].get<std::string>();
            if (allowed.find(kind) == allowed.end())
            {
                if (!conflicts.empty()) conflicts += "、";
                conflicts += row[0].get<std::string>();
            }
        }
        if (!conflicts.empty())
        {
            return ResponseHelper::validation(req, "以下职位的工种不属于新业务域，请先调整或删除：" + conflicts);
        }
    }

    try
    {
        dbManager->getSession()
            ->sql("UPDATE departments SET name = ?, business_domain = ? WHERE id = ?")
            .bind(name, businessDomain, departmentId)
            .execute();
    }
    catch (const mysqlx::Error &)
    {
        // departments.name 唯一约束：重名给干净的 400 而不是 500
        return ResponseHelper::validation(req, "部门名称已存在");
    }
    return ResponseHelper::success(req, {{"id", departmentId}, {"name", name}, {"business_domain", businessDomain}});
}

crow::response deleteDepartment(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, int departmentId)
{
    mysqlx::Row current = dbManager->getSession()
                              ->sql("SELECT name, COALESCE(is_system, 0) FROM departments WHERE id = ? LIMIT 1")
                              .bind(departmentId)
                              .execute()
                              .fetchOne();
    if (!current)
    {
        return ResponseHelper::notFound(req, "部门不存在");
    }
    // 系统种子部门是权限边界与业务身份的锚点，与编辑同一条纪律：一律不可删除。
    if (current[1].get<int>() != 0)
    {
        return ResponseHelper::validation(req, "系统内置部门不可删除");
    }
    // 部门下仍有职位时，positions.department_id 外键会阻止删除；先给出清晰的中文引导。
    mysqlx::Row positionCount = dbManager->getSession()
                                    ->sql("SELECT COUNT(*) FROM positions WHERE department_id = ?")
                                    .bind(departmentId)
                                    .execute()
                                    .fetchOne();
    if (positionCount && positionCount[0].get<int>() > 0)
    {
        return ResponseHelper::validation(req, "该部门下仍有 " + std::to_string(positionCount[0].get<int>()) + " 个职位，请先删除或迁移这些职位");
    }

    try
    {
        dbManager->getSession()
            ->sql("DELETE FROM departments WHERE id = ?")
            .bind(departmentId)
            .execute();
    }
    catch (const mysqlx::Error &)
    {
        // 考勤记录 / 组织范围 / 操作日志等历史数据仍引用该部门时，外键会阻止删除。
        return ResponseHelper::validation(req, "该部门仍被历史数据（如考勤、组织范围记录）引用，无法删除");
    }
    return ResponseHelper::success(req, {{"id", departmentId}});
}

crow::response getPositions(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager)
{
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT p.id, p.department_id, COALESCE(d.name, ''), p.name, "
                                        "COALESCE(p.system_key, ''), COALESCE(p.staff_kind, ''), COALESCE(p.description, ''), "
                                        "COALESCE(p.assignment_policy, 'super_admin_only') "
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
    // 工种必须属于所在部门的业务域（普通员工在任何部门可选），杜绝「管理部·财务」这类矛盾组合。
    {
        mysqlx::Row domainRow = dbManager->getSession()
                                    ->sql("SELECT COALESCE(business_domain, 'general') FROM departments WHERE id = ? LIMIT 1")
                                    .bind(departmentId.value())
                                    .execute()
                                    .fetchOne();
        const std::string domain = domainRow ? domainRow[0].get<std::string>() : "general";
        const std::set<std::string> &allowed = staffKindsForDomain(domain);
        if (allowed.find(staffKind) == allowed.end())
        {
            return ResponseHelper::validation(req, "该工种不属于所选部门的业务域，请选择与部门匹配的工种");
        }
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

crow::response deletePosition(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, int positionId)
{
    mysqlx::Row current = dbManager->getSession()
                              ->sql("SELECT name, COALESCE(system_key, '') FROM positions WHERE id = ? LIMIT 1")
                              .bind(positionId)
                              .execute()
                              .fetchOne();
    if (!current)
    {
        return ResponseHelper::notFound(req, "岗位不存在");
    }
    // 带 system_key 的都是系统/seed 锚点职位（含超管），业务身份 findPositionIdBySystemKey 依赖它们，禁止删除。
    if (!current[1].get<std::string>().empty())
    {
        return ResponseHelper::validation(req, "系统内置职位不可删除");
    }
    // 仍有在职职工挂在该职位时，users.position_id 外键会阻止删除；先给出可操作的提示。
    mysqlx::Row activeUsers = dbManager->getSession()
                                  ->sql("SELECT COUNT(*) FROM users WHERE position_id = ? AND is_deleted = 0")
                                  .bind(positionId)
                                  .execute()
                                  .fetchOne();
    if (activeUsers && activeUsers[0].get<int>() > 0)
    {
        return ResponseHelper::validation(req, "该职位下仍有 " + std::to_string(activeUsers[0].get<int>()) + " 名在职职工，请先调岗后再删除");
    }

    // position_permissions 由外键 ON DELETE CASCADE 自动清理；此处只删职位本身。
    try
    {
        dbManager->getSession()
            ->sql("DELETE FROM positions WHERE id = ?")
            .bind(positionId)
            .execute();
    }
    catch (const mysqlx::Error &)
    {
        // 已离职职工、考勤记录等历史数据仍引用该职位时，外键会阻止删除。
        return ResponseHelper::validation(req, "该职位仍被历史数据（如离职职工、考勤记录）引用，无法删除");
    }
    return ResponseHelper::success(req, {{"id", positionId}});
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
    nlohmann::json grantableKeys = nlohmann::json::array();
    const auto allowed = RbacService::allowedDomainsForPosition(dbManager, positionId);
    for (const auto &key : Permissions::grantablePermissionKeys())
    {
        if (allowed.count(Permissions::domainOfPermission(key))) grantableKeys.push_back(key);
    }
    const auto policy = PositionPermissionService::loadAssignmentPolicy(dbManager, positionId);
    // 地板按原始权限行计算（含未知 key → SuperAdminOnly），与写路径一致。
    std::vector<std::string> rawKeys;
    {
        mysqlx::SqlResult raw = dbManager->getSession()
                                    ->sql("SELECT permission_key FROM position_permissions WHERE position_id = ?")
                                    .bind(positionId)
                                    .execute();
        for (mysqlx::Row r = raw.fetchOne(); r; r = raw.fetchOne())
        {
            if (!r[0].isNull())
            {
                rawKeys.push_back(r[0].get<std::string>());
            }
        }
    }
    const auto floor = Permissions::requiredAssignmentPolicy(rawKeys);
    return ResponseHelper::success(req, {
        {"position_id", positionId},
        {"permissions", permissions},
        {"grantableKeys", grantableKeys},
        {"assignment_policy", Permissions::assignmentPolicyKey(policy)},
        {"minimum_assignment_policy", Permissions::assignmentPolicyKey(floor)},
    });
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

    const auto rejected = permissionsOutsideAllowedDomains(dbManager, positionId, permissions.value());
    if (!rejected.empty()) return domainBoundaryError(req, rejected);

    std::optional<std::string> requestedPolicy;
    if (body.contains("assignment_policy") && body["assignment_policy"].is_string())
    {
        requestedPolicy = body["assignment_policy"].get<std::string>();
    }

    const auto result = PositionPermissionService::replacePermissions(
        dbManager, positionId, permissions.value(), requestedPolicy);
    if (!result.ok)
    {
        if (result.errorCode == "NOT_FOUND")
        {
            return ResponseHelper::notFound(req, result.errorMessage);
        }
        if (result.errorCode == "SUPER_ADMIN_LOCKED" || result.errorCode == "POLICY_BELOW_FLOOR")
        {
            return ResponseHelper::permission_denied(req, result.errorMessage, result.errorCode);
        }
        return ResponseHelper::validation(req, result.errorMessage);
    }

    bumpUsersInPosition(dbManager, positionId);
    AccessRevocation::closeRealtimeConnections();
    return ResponseHelper::success(req, {
        {"position_id", positionId},
        {"permissions", result.permissions},
        {"assignment_policy", Permissions::assignmentPolicyKey(result.effectivePolicy)},
        {"minimum_assignment_policy", Permissions::assignmentPolicyKey(result.requiredFloor)},
    });
}

crow::response getUserPermissions(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, int userId)
{
    // LEFT JOIN：普通用户尚未派岗时仍可加载个人授权页，不能误报“用户不存在”。
    mysqlx::Row user = dbManager->getSession()->sql(
        "SELECT u.position_id, COALESCE(p.name, ''), COALESCE(d.name, '') FROM users u "
        "LEFT JOIN positions p ON p.id=u.position_id LEFT JOIN departments d ON d.id=p.department_id "
        "WHERE u.id=? AND u.is_deleted=0 LIMIT 1").bind(userId).execute().fetchOne();
    if (!user) return ResponseHelper::notFound(req, "用户不存在");
    const int positionId = user[0].isNull() ? 0 : user[0].get<int>();
    nlohmann::json positionPermissions = nlohmann::json::array();
    if (positionId > 0)
    {
        positionPermissions = RbacService::loadPermissionsForPosition(dbManager, positionId);
    }
    nlohmann::json personalPermissions = nlohmann::json::array();
    mysqlx::SqlResult personal = dbManager->getSession()->sql(
        "SELECT permission_key FROM user_permissions WHERE user_id=? ORDER BY permission_key").bind(userId).execute();
    for (mysqlx::Row row = personal.fetchOne(); row; row = personal.fetchOne())
        if (!row[0].isNull() && Permissions::isKnownPermissionKey(row[0].get<std::string>())) personalPermissions.push_back(row[0].get<std::string>());
    nlohmann::json grantableKeys = nlohmann::json::array();
    if (positionId > 0)
    {
        const auto allowed = RbacService::allowedDomainsForPosition(dbManager, positionId);
        for (const auto &key : Permissions::grantablePermissionKeys())
            if (allowed.count(Permissions::domainOfPermission(key))) grantableKeys.push_back(key);
    }
    return ResponseHelper::success(req, {
        {"positionId", positionId > 0 ? nlohmann::json(positionId) : nlohmann::json(nullptr)},
        {"positionName", user[1].get<std::string>()},
        {"departmentName", user[2].get<std::string>()},
        {"positionPermissions", positionPermissions},
        {"personalPermissions", personalPermissions},
        {"grantableKeys", grantableKeys}});
}

crow::response updateUserPermissions(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager,
                                     int operatorId, int userId, const nlohmann::json &body)
{
    mysqlx::Row user = dbManager->getSession()->sql(
        "SELECT position_id FROM users WHERE id=? AND is_deleted=0 LIMIT 1").bind(userId).execute().fetchOne();
    if (!user) return ResponseHelper::notFound(req, "用户不存在");
    crow::response validationRes;
    auto permissions = parseGrantablePermissions(req, body, validationRes);
    if (!permissions) return validationRes;
    const auto rejected = permissionsOutsideAllowedDomains(dbManager, user[0].get<int>(), *permissions);
    if (!rejected.empty()) return domainBoundaryError(req, rejected);
    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        session->sql("DELETE FROM user_permissions WHERE user_id=?").bind(userId).execute();
        for (const auto &key : *permissions)
            session->sql("INSERT INTO user_permissions (user_id, permission_key, granted_by) VALUES (?, ?, ?)")
                .bind(userId, key, operatorId).execute();
        session->sql("COMMIT").execute();
    }
    catch (...) { rollbackTransactionQuietly(*session); throw; }
    AccessRevocation::onUserAccessChanged(userId);
    return ResponseHelper::success(req, {{"userId", userId}, {"permissions", *permissions}});
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

    const auto result = PositionPermissionService::replacePermissions(dbManager, positionId, permissions, std::nullopt);
    if (!result.ok)
    {
        if (result.errorCode == "NOT_FOUND")
        {
            return ResponseHelper::notFound(req, result.errorMessage);
        }
        if (result.errorCode == "SUPER_ADMIN_LOCKED" || result.errorCode == "POLICY_BELOW_FLOOR")
        {
            return ResponseHelper::permission_denied(req, result.errorMessage, result.errorCode);
        }
        return ResponseHelper::validation(req, result.errorMessage);
    }
    bumpUsersInPosition(dbManager, positionId);
    AccessRevocation::closeRealtimeConnections();
    return ResponseHelper::success(req, {
        {"position_id", positionId},
        {"template_id", templateId.value()},
        {"permissions", result.permissions},
        {"assignment_policy", Permissions::assignmentPolicyKey(result.effectivePolicy)},
        {"minimum_assignment_policy", Permissions::assignmentPolicyKey(result.requiredFloor)},
    });
}

crow::response resetPositionDefaults(const crow::request &req, const std::shared_ptr<DatabaseManagerInterface> &dbManager, int positionId)
{
    mysqlx::Row position = dbManager->getSession()->sql(
        "SELECT staff_kind, COALESCE(system_key, '') FROM positions WHERE id=? LIMIT 1").bind(positionId).execute().fetchOne();
    if (!position) return ResponseHelper::notFound(req, "岗位不存在");
    const std::string kind = position[0].get<std::string>();
    const std::string systemKey = position[1].get<std::string>();
    std::string templateName;
    if (systemKey == "president" || systemKey == "vice-president") templateName = "Boss";
    else if (systemKey == "super-admin") templateName = "SuperAdmin";
    else if (kind == "doctor" || kind == "nurse") templateName = "Medical";
    else if (kind == "finance") templateName = "Finance";
    else if (kind == "personnel") templateName = "Personnel";
    else if (kind == "warehouse") templateName = "Warehouse";

    std::vector<std::string> permissions;
    if (!templateName.empty())
    {
        mysqlx::Row tmpl = dbManager->getSession()->sql(
            "SELECT id FROM permission_templates WHERE name=? LIMIT 1").bind(templateName).execute().fetchOne();
        if (!tmpl) return ResponseHelper::notFound(req, "默认权限模板不存在");
        permissions = loadTemplatePermissions(dbManager, tmpl[0].get<int>());
    }
    const auto result = PositionPermissionService::replacePermissions(dbManager, positionId, permissions, std::nullopt);
    if (!result.ok)
    {
        if (result.errorCode == "NOT_FOUND")
        {
            return ResponseHelper::notFound(req, result.errorMessage);
        }
        if (result.errorCode == "SUPER_ADMIN_LOCKED")
        {
            return ResponseHelper::permission_denied(req, result.errorMessage, result.errorCode);
        }
        return ResponseHelper::validation(req, result.errorMessage);
    }
    bumpUsersInPosition(dbManager, positionId);
    AccessRevocation::closeRealtimeConnections();
    return ResponseHelper::success(req, {
        {"position_id", positionId},
        {"permissions", result.permissions},
        {"assignment_policy", Permissions::assignmentPolicyKey(result.effectivePolicy)},
    });
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

    EmploymentAssignmentService::AssignRequest assignReq;
    assignReq.operatorUserId = operatorUserId;
    assignReq.targetUserId = targetUserId;
    assignReq.mode = EmploymentAssignmentService::ActorMode::Admin;
    assignReq.reason = jsonString(body, "reason");
    // B15: break-glass/管理派岗必须显式提供 reason，禁止默认兜底文本。
    if (assignReq.reason.empty())
    {
        return ResponseHelper::validation(req, "reason 不能为空");
    }
    // Admin 任职入口即 break-glass 直派（仍须 rbac:manage + reason）。
    assignReq.explicitBreakGlass = true;
    assignReq.effectiveFrom = jsonString(body, "effective_from");

    if (!body.contains("expected_current_position_id") ||
        (!body["expected_current_position_id"].is_number_integer() && !body["expected_current_position_id"].is_null()))
    {
        // null 表示期望客户(0)；整数表示期望职位
        if (!(body.contains("expected_current_position_id") && body["expected_current_position_id"].is_null()))
        {
            return ResponseHelper::validation(req, "expected_current_position_id 必填");
        }
    }
    if (!body.contains("expected_current_position_id"))
    {
        return ResponseHelper::validation(req, "expected_current_position_id 必填");
    }
    if (body["expected_current_position_id"].is_null())
    {
        assignReq.expectedCurrentPositionId = 0;
    }
    else
    {
        assignReq.expectedCurrentPositionId = body["expected_current_position_id"].get<int>();
    }
    assignReq.hasExpectedCurrentPosition = true;

    const std::optional<int> positionId = jsonInt(body, "position_id");
    const bool assignCustomer = !positionId.has_value() || positionId.value() <= 0;
    if (assignCustomer)
    {
        assignReq.action = EmploymentAssignmentService::Action::Offboard;
    }
    else
    {
        assignReq.targetPositionId = positionId.value();
        if (assignReq.expectedCurrentPositionId > 0)
        {
            assignReq.action = EmploymentAssignmentService::Action::Transfer;
        }
        else
        {
            assignReq.action = EmploymentAssignmentService::Action::Onboard;
        }
    }

    const auto result = EmploymentAssignmentService::assign(dbManager, assignReq);
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
                ResponseErrorType::BusinessConflict,
                result.errorCode.empty() ? result.message : result.errorCode);
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
    });
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

    CROW_ROUTE(app, "/api/admin/org/departments/<int>")
        .methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int departmentId)
            {
                int userId = -1;
                const bool isDelete = req.method == crow::HTTPMethod::Delete;
                const std::string action = isDelete ? "删除部门" : "编辑部门";
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
                    if (isDelete)
                    {
                        res = deleteDepartment(req, dbManager, departmentId);
                    }
                    else
                    {
                        BaseHandler parser(dbManager);
                        auto jsonOpt = parser.parseJson(req, res);
                        if (!jsonOpt)
                        {
                            OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt);
                            return;
                        }
                        res = updateDepartment(req, dbManager, departmentId, jsonOpt.value());
                    }
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt, true);
            });

    CROW_ROUTE(app, "/api/admin/org/positions/<int>")
        .methods(crow::HTTPMethod::Delete, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int positionId)
            {
                int userId = -1;
                const std::string action = "删除职位";
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
                    res = deletePosition(req, dbManager, positionId);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt, true);
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

    CROW_ROUTE(app, "/api/admin/rbac/positions/<int>/reset-defaults")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int positionId)
            {
                int userId = isValidPermissionToken(req, res, dbManager, Permissions::kRbacManage);
                const std::string action = "重置岗位默认权限";
                if (res.code != 200 || userId == -1) { OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action); return; }
                try { res = resetPositionDefaults(req, dbManager, positionId); }
                catch (const std::exception &e) { OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId); res = ResponseHelper::system_error(req); }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", action, Permissions::kRbacManage, userId);
            });

    CROW_ROUTE(app, "/api/admin/rbac/users/<int>/permissions")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Put, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int targetUserId)
            {
                const bool isUpdate = req.method == crow::HTTPMethod::Put;
                const std::string action = isUpdate ? "修改职工个人权限" : "获取职工个人权限";
                int userId = isUpdate ? isValidPermissionToken(req, res, dbManager, Permissions::kRbacManage)
                                      : isValidManagementToken(req, res, dbManager);
                if (res.code != 200 || userId == -1) { OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action); return; }
                try
                {
                    if (isUpdate)
                    {
                        BaseHandler parser(dbManager); auto body = parser.parseJson(req, res);
                        if (body) res = updateUserPermissions(req, dbManager, userId, targetUserId, *body);
                    }
                    else res = getUserPermissions(req, dbManager, targetUserId);
                }
                catch (const std::exception &e) { OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId); res = ResponseHelper::system_error(req); }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", action, Permissions::kRbacManage, userId);
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

    CROW_ROUTE(app, "/api/admin/report-templates/<int>/versions/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int templateId, int versionId)
        {
            const int userId = isValidPermissionToken(req, res, dbManager, Permissions::kReportTemplateRead);
            if (res.code != 200 || userId <= 0)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "读取打印模板正文");
                return;
            }
            ReportTemplateHandler handler(dbManager);
            auto response = handler.getVersion(req, templateId, versionId);
            ProcessHandlerResponse(req, res, response);
            OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "读取打印模板正文", Permissions::kReportTemplateRead, userId);
        });

    CROW_ROUTE(app, "/api/admin/report-templates/<int>/previews")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int templateId)
        {
            const int userId = isValidPermissionToken(req, res, dbManager, Permissions::kReportTemplateManage);
            if (res.code != 200 || userId <= 0)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "预览打印模板");
                return;
            }
            ReportTemplateHandler handler(dbManager);
            auto response = handler.preview(req, templateId);
            ProcessHandlerResponse(req, res, response);
            OperationLogger::FinishSensitiveRoute(dbManager, req, res, "管理", "预览打印模板", Permissions::kReportTemplateManage, userId);
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
