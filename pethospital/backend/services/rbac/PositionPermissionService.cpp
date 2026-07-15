#include "PositionPermissionService.h"

#include "../../utils/Utils.h"

#include <iostream>
#include <vector>

namespace PositionPermissionService
{
namespace
{
ReplaceResult fail(const std::string &message, const std::string &code = "")
{
    ReplaceResult result;
    result.ok = false;
    result.errorMessage = message;
    result.errorCode = code;
    return result;
}

// 原始权限行，不过滤未知 key —— 未知 key 必须把 floor 抬到 SuperAdminOnly。
std::vector<std::string> loadRawPermissionKeys(mysqlx::Session &session, int positionId)
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

ReplaceResult applyInTransaction(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId,
    const std::vector<std::string> *permissionsOrNull,
    const std::optional<std::string> &requestedPolicy)
{
    if (!dbManager || !dbManager->getSession() || positionId <= 0)
    {
        return fail("数据库不可用");
    }

    if (permissionsOrNull != nullptr)
    {
        for (const std::string &key : *permissionsOrNull)
        {
            if (!Permissions::isGrantablePermissionKey(key))
            {
                return fail("包含不可授予或未知权限: " + key, "INVALID_PERMISSION");
            }
        }
    }
    if (requestedPolicy.has_value() && !Permissions::isValidAssignmentPolicyKey(requestedPolicy.value()))
    {
        return fail("assignment_policy 取值不合法", "INVALID_POLICY");
    }

    auto session = dbManager->getSession();
    session->sql("START TRANSACTION").execute();
    try
    {
        // 锁职位行：system_key / 当前 policy 与后续写在同一事务窗口内。
        mysqlx::Row positionRow = session->sql(
                                             "SELECT COALESCE(system_key, ''), "
                                             "COALESCE(assignment_policy, 'super_admin_only') "
                                             "FROM positions WHERE id = ? LIMIT 1 FOR UPDATE")
                                      .bind(positionId)
                                      .execute()
                                      .fetchOne();
        if (!positionRow)
        {
            rollbackTransactionQuietly(*session);
            return fail("岗位不存在", "NOT_FOUND");
        }

        const std::string systemKey = positionRow[0].isNull() ? "" : positionRow[0].get<std::string>();
        if (systemKey == "super-admin")
        {
            rollbackTransactionQuietly(*session);
            return fail("系统超级管理员岗位权限不允许修改", "SUPER_ADMIN_LOCKED");
        }

        const Permissions::AssignmentPolicy currentPolicy =
            Permissions::parseAssignmentPolicy(
                positionRow[1].isNull() ? "super_admin_only" : positionRow[1].get<std::string>());

        std::vector<std::string> permissions;
        if (permissionsOrNull != nullptr)
        {
            permissions = *permissionsOrNull;
        }
        else
        {
            // 策略-only 路径：用原始行算 floor（含未知 key）。
            permissions = loadRawPermissionKeys(*session, positionId);
        }

        const Permissions::AssignmentPolicy floor =
            Permissions::requiredAssignmentPolicy(permissions);

        Permissions::AssignmentPolicy finalPolicy = currentPolicy;
        if (requestedPolicy.has_value())
        {
            finalPolicy = Permissions::parseAssignmentPolicy(requestedPolicy.value());
            if (Permissions::assignmentPolicyRank(finalPolicy) < Permissions::assignmentPolicyRank(floor))
            {
                rollbackTransactionQuietly(*session);
                return fail(
                    std::string("派岗策略不能低于权限安全下限 ") + Permissions::assignmentPolicyKey(floor),
                    "POLICY_BELOW_FLOOR");
            }
        }
        else
        {
            finalPolicy = Permissions::maxAssignmentPolicy(currentPolicy, floor);
        }

        if (permissionsOrNull != nullptr)
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
        }

        session->sql("UPDATE positions SET assignment_policy = ? WHERE id = ?")
            .bind(Permissions::assignmentPolicyKey(finalPolicy), positionId)
            .execute();

        session->sql("COMMIT").execute();

        // 返回给调用方的权限列表只含可授予/已知键（写入集）；floor 已按写入/原始集计算。
        ReplaceResult result;
        result.ok = true;
        result.effectivePolicy = finalPolicy;
        result.requiredFloor = floor;
        if (permissionsOrNull != nullptr)
        {
            result.permissions = std::move(permissions);
        }
        else
        {
            // 策略-only：回传过滤后的已知键供响应展示
            for (const std::string &key : permissions)
            {
                if (Permissions::isKnownPermissionKey(key) && key != Permissions::kRbacManage)
                {
                    result.permissions.push_back(key);
                }
            }
        }
        return result;
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        std::cerr << "PositionPermissionService transaction failed: " << e.what() << std::endl;
        return fail("更新职位权限失败");
    }
}
}

Permissions::AssignmentPolicy loadAssignmentPolicy(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId)
{
    if (!dbManager || !dbManager->getSession() || positionId <= 0)
    {
        return Permissions::AssignmentPolicy::SuperAdminOnly;
    }
    try
    {
        mysqlx::Row row = dbManager->getSession()
                              ->sql("SELECT COALESCE(assignment_policy, 'super_admin_only') "
                                    "FROM positions WHERE id = ? LIMIT 1")
                              .bind(positionId)
                              .execute()
                              .fetchOne();
        if (!row || row[0].isNull())
        {
            return Permissions::AssignmentPolicy::SuperAdminOnly;
        }
        return Permissions::parseAssignmentPolicy(row[0].get<std::string>());
    }
    catch (const std::exception &e)
    {
        std::cerr << "PositionPermissionService::loadAssignmentPolicy failed: " << e.what() << std::endl;
        return Permissions::AssignmentPolicy::SuperAdminOnly;
    }
}

ReplaceResult replacePermissions(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId,
    const std::vector<std::string> &permissions,
    const std::optional<std::string> &requestedPolicy)
{
    return applyInTransaction(dbManager, positionId, &permissions, requestedPolicy);
}

ReplaceResult updateAssignmentPolicy(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int positionId,
    const std::string &requestedPolicy)
{
    return applyInTransaction(dbManager, positionId, nullptr, requestedPolicy);
}
}
