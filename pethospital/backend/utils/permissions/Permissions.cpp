#include "Permissions.h"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <stdexcept>

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
    kSalaryReview,
    kSalarySubmitReview,
    kSalarySupervisorReview,
    kSalaryLock,
    kLogsRead,
    kMedicalRecordRead,
    kMedicalRecordWrite,
    kMedicalRecordFinalize,
    kMedicalRecordPrint,
    kMedicalRecordAmend,
    kMedicalRecordVoid,
    kReportTemplateRead,
    kReportTemplateManage,
    kReportTemplatePublish,
    kDoctorWorkWrite,
    kUserDelete,
    kEquityRead,
    kEquityWrite,
    kStockRead,
    kStockWrite,
    kStaffRoleWrite,
    kAttendanceRead,
    kAttendanceManage,
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

PermissionDomain domainOfPermission(const std::string &key)
{
    if (key == kPortalUser || key == kAttendanceRead) return PermissionDomain::General;
    if (key == kPortalMedical || key == kMedicalRecordRead || key == kMedicalRecordWrite ||
        key == kMedicalRecordFinalize || key == kMedicalRecordPrint || key == kMedicalRecordAmend ||
        key == kMedicalRecordVoid || key == kDoctorWorkWrite || key == kScopeMedicalAssigned)
        return PermissionDomain::Medical;
    if (key == kPortalFinance || key == kSalaryRead || key == kSalaryWrite || key == kSalaryReview ||
        key == kSalarySubmitReview || key == kSalarySupervisorReview || key == kSalaryLock ||
        key == kEquityRead || key == kEquityWrite) return PermissionDomain::Finance;
    if (key == kPortalPersonnel || key == kStaffRoleWrite || key == kAttendanceManage)
        return PermissionDomain::Personnel;
    if (key == kPortalWarehouse || key == kStockRead || key == kStockWrite)
        return PermissionDomain::Warehouse;
    if (key == kPortalBoss || key == kPortalSuperAdmin || key == kLogsRead || key == kUserDelete ||
        key == kRbacManage || key == kScopeAll || key == kReportTemplateRead ||
        key == kReportTemplateManage || key == kReportTemplatePublish) return PermissionDomain::Management;
    throw std::invalid_argument("Unknown permission key: " + key);
}

std::string domainKey(PermissionDomain domain)
{
    switch (domain) {
    case PermissionDomain::General: return "general";
    case PermissionDomain::Medical: return "medical";
    case PermissionDomain::Finance: return "finance";
    case PermissionDomain::Personnel: return "personnel";
    case PermissionDomain::Warehouse: return "warehouse";
    case PermissionDomain::Management: return "management";
    }
    return "general";
}

std::string domainChineseName(PermissionDomain domain)
{
    switch (domain) {
    case PermissionDomain::General: return "通用";
    case PermissionDomain::Medical: return "医疗";
    case PermissionDomain::Finance: return "财务";
    case PermissionDomain::Personnel: return "人事";
    case PermissionDomain::Warehouse: return "仓储";
    case PermissionDomain::Management: return "管理";
    }
    return "通用";
}
}
