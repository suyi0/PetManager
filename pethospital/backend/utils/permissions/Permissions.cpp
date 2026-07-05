#include "Permissions.h"

#include <algorithm>
#include <initializer_list>

namespace Permissions
{
namespace
{
bool hasPermission(std::initializer_list<const char *> permissions, const std::string &permissionKey)
{
    return std::find_if(
               permissions.begin(),
               permissions.end(),
               [&permissionKey](const char *candidate)
               {
                   return permissionKey == candidate;
               }) != permissions.end();
}

bool isBossPackage(const std::string &roleName)
{
    return roleName == "总裁" || roleName == "副总裁";
}

bool isFinancePackage(const std::string &roleName)
{
    return roleName == "财务总监" || roleName == "财务经理";
}

bool isSuperAdminPackage(const std::string &roleName)
{
    return roleName == "部门经理" || roleName == "超级管理员";
}
}

bool roleHasPermission(const std::string &roleName, const std::string &permissionKey)
{
    if (roleName.empty() || permissionKey.empty())
    {
        return false;
    }

    if (isBossPackage(roleName))
    {
        return hasPermission(
            {kPortalBoss,
             kPortalFinance,
             kPortalSuperAdmin,
             kPortalPersonnel,
             kPortalMedical,
             kPortalWarehouse,
             kSalaryRead,
             kSalaryWrite,
             kLogsRead,
             kMedicalRecordRead,
             kMedicalRecordWrite,
             kDoctorWorkWrite,
             kUserDelete,
             kEquityRead,
             kEquityWrite,
             kStockRead,
             kStockWrite,
             kStaffRoleWrite,
             kScopeAll},
            permissionKey);
    }

    if (isFinancePackage(roleName))
    {
        return hasPermission(
            {kPortalFinance,
             kSalaryRead,
             kSalaryWrite},
            permissionKey);
    }

    if (isSuperAdminPackage(roleName))
    {
        return hasPermission(
            {kPortalSuperAdmin,
             kLogsRead,
             kMedicalRecordRead,
             kDoctorWorkWrite,
             kUserDelete},
            permissionKey);
    }

    if (roleName == "人事经理")
    {
        return hasPermission({kPortalPersonnel, kStaffRoleWrite}, permissionKey);
    }

    if (roleName == "医生" || roleName == "护士")
    {
        return hasPermission(
            {kPortalMedical,
             kMedicalRecordWrite,
             kScopeMedicalAssigned},
            permissionKey);
    }

    if (roleName == "仓库管理员")
    {
        return hasPermission(
            {kPortalWarehouse,
             kStockRead,
             kStockWrite},
            permissionKey);
    }

    return false;
}
}
