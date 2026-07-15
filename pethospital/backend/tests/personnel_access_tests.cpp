#include "../services/employment/PersonnelAccess.h"
#include "../utils/permissions/Permissions.h"

#include <cassert>
#include <string>

// 无 DB 的纯逻辑契约：action 解析与权限键映射。
// 人事域双读依赖 DB 的路径由 employment_assignment_contract_tests 锁源码。
int main()
{
    PersonnelAccess::AssignmentAction action = PersonnelAccess::AssignmentAction::Read;

    assert(PersonnelAccess::parseAssignmentAction("onboard", action));
    assert(action == PersonnelAccess::AssignmentAction::Onboard);
    assert(std::string(PersonnelAccess::permissionKeyForAction(action)) == Permissions::kEmploymentOnboard);

    assert(PersonnelAccess::parseAssignmentAction("transfer", action));
    assert(action == PersonnelAccess::AssignmentAction::Transfer);
    assert(std::string(PersonnelAccess::permissionKeyForAction(action)) == Permissions::kEmploymentAssign);

    assert(PersonnelAccess::parseAssignmentAction("offboard", action));
    assert(action == PersonnelAccess::AssignmentAction::Offboard);
    assert(std::string(PersonnelAccess::permissionKeyForAction(action)) == Permissions::kEmploymentOffboard);

    // regularize / 未知不得被 assignment 接口接受
    assert(!PersonnelAccess::parseAssignmentAction("regularize", action));
    assert(!PersonnelAccess::parseAssignmentAction("", action));
    assert(!PersonnelAccess::parseAssignmentAction("hack", action));

    assert(std::string(PersonnelAccess::permissionKeyForAction(PersonnelAccess::AssignmentAction::Read)) ==
           Permissions::kEmploymentRead);

    // fail-closed：无 DB / 非法 userId
    assert(!PersonnelAccess::operatorHoldsPersonnelDomainPosition(nullptr, 1));
    assert(!PersonnelAccess::canPerformAssignmentAction(nullptr, 1, PersonnelAccess::AssignmentAction::Onboard));
    assert(!PersonnelAccess::canReadEmployment(nullptr, 1));
    assert(!PersonnelAccess::canPerformAssignmentAction(nullptr, 0, PersonnelAccess::AssignmentAction::Offboard));

    return 0;
}
