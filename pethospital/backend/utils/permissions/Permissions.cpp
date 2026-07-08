#include "Permissions.h"

#include <algorithm>
#include <array>
#include <initializer_list>

namespace Permissions
{
namespace
{
const std::array kAllPermissionKeys = {
    kPortalBoss,
    kPortalFinance,
    kPortalSuperAdmin,
    kPortalPersonnel,
    kPortalMedical,
    kPortalWarehouse,
    kPortalUser,
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
    kScopeAll,
    kScopeMedicalAssigned,
    kRbacManage,
};

std::vector<std::string> copyKeysExceptMetaPermission()
{
    std::vector<std::string> keys;
    keys.reserve(kAllPermissionKeys.size() - 1);
    for (const char *key : kAllPermissionKeys)
    {
        if (std::string(key) != kRbacManage)
        {
            keys.emplace_back(key);
        }
    }
    return keys;
}
}

std::vector<std::string> allPermissionKeys()
{
    return {kAllPermissionKeys.begin(), kAllPermissionKeys.end()};
}

std::vector<std::string> grantablePermissionKeys()
{
    return copyKeysExceptMetaPermission();
}

bool isKnownPermissionKey(const std::string &permissionKey)
{
    return std::find(kAllPermissionKeys.begin(), kAllPermissionKeys.end(), permissionKey) != kAllPermissionKeys.end();
}

bool isGrantablePermissionKey(const std::string &permissionKey)
{
    return permissionKey != kRbacManage && isKnownPermissionKey(permissionKey);
}
}
