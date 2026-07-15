#include "PersonnelAccess.h"

#include "../rbac/RbacService.h"

#include <iostream>

namespace PersonnelAccess
{
namespace
{
bool hasPermission(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId,
    const std::string &key)
{
    return RbacService::userHasPermission(dbManager, userId, key);
}
}

bool parseAssignmentAction(const std::string &action, AssignmentAction &out)
{
    if (action == "onboard")
    {
        out = AssignmentAction::Onboard;
        return true;
    }
    if (action == "transfer")
    {
        out = AssignmentAction::Transfer;
        return true;
    }
    if (action == "offboard")
    {
        out = AssignmentAction::Offboard;
        return true;
    }
    // regularize 不进入 assignment PUT
    return false;
}

const char *permissionKeyForAction(AssignmentAction action)
{
    switch (action)
    {
    case AssignmentAction::Read:
        return Permissions::kEmploymentRead;
    case AssignmentAction::Onboard:
        return Permissions::kEmploymentOnboard;
    case AssignmentAction::Transfer:
        return Permissions::kEmploymentAssign;
    case AssignmentAction::Offboard:
        return Permissions::kEmploymentOffboard;
    case AssignmentAction::Regularize:
        return Permissions::kEmploymentRegularize;
    }
    return Permissions::kEmploymentRead;
}

bool operatorHoldsPersonnelDomainPosition(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId)
{
    if (!dbManager || !dbManager->getSession() || operatorUserId <= 0)
    {
        return false;
    }
    try
    {
        // 人事域：职位 staff_kind=personnel，或所在部门 business_domain=personnel。
        mysqlx::Row row = dbManager->getSession()
                              ->sql("SELECT 1 FROM users u "
                                    "JOIN positions p ON p.id = u.position_id "
                                    "LEFT JOIN departments d ON d.id = p.department_id "
                                    "WHERE u.id = ? AND u.is_deleted = 0 "
                                    "AND (p.staff_kind = 'personnel' "
                                    "     OR COALESCE(d.business_domain, '') = 'personnel') "
                                    "LIMIT 1")
                              .bind(operatorUserId)
                              .execute()
                              .fetchOne();
        return static_cast<bool>(row);
    }
    catch (const std::exception &e)
    {
        std::cerr << "PersonnelAccess::operatorHoldsPersonnelDomainPosition: " << e.what() << std::endl;
        return false;
    }
}

bool canPerformAssignmentAction(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId,
    AssignmentAction action)
{
    if (operatorUserId <= 0)
    {
        return false;
    }
    const std::string primary = permissionKeyForAction(action);
    if (hasPermission(dbManager, operatorUserId, primary))
    {
        return true;
    }
    // 旧 key 双读：仅人事域职位 + staff-role:write，且语义覆盖该 action（迁移期整包映射）。
    if (!operatorHoldsPersonnelDomainPosition(dbManager, operatorUserId))
    {
        return false;
    }
    return hasPermission(dbManager, operatorUserId, Permissions::kStaffRoleWrite);
}

bool canReadEmployment(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int operatorUserId)
{
    return canPerformAssignmentAction(dbManager, operatorUserId, AssignmentAction::Read);
}
}
