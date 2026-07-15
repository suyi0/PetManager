#include "Permissions.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace Permissions
{
namespace
{
// 每个已知 key 必须显式列出 domain + minimumPolicy。
// 禁止「已知但未列出 → PersonnelDirect」的隐式兜底；漏标 = 编译期/测试失败。
constexpr PermissionCatalogEntry kCatalog[] = {
    // portals
    {kPortalBoss, PermissionDomain::Management, AssignmentPolicy::ApprovalRequired},
    {kPortalFinance, PermissionDomain::Finance, AssignmentPolicy::ApprovalRequired},
    {kPortalSuperAdmin, PermissionDomain::Management, AssignmentPolicy::ApprovalRequired},
    {kPortalPersonnel, PermissionDomain::Personnel, AssignmentPolicy::ApprovalRequired},
    {kPortalMedical, PermissionDomain::Medical, AssignmentPolicy::PersonnelDirect},
    {kPortalWarehouse, PermissionDomain::Warehouse, AssignmentPolicy::PersonnelDirect},
    {kPortalUser, PermissionDomain::General, AssignmentPolicy::PersonnelDirect},
    // salary / finance
    {kSalaryRead, PermissionDomain::Finance, AssignmentPolicy::ApprovalRequired},
    {kSalaryWrite, PermissionDomain::Finance, AssignmentPolicy::ApprovalRequired},
    {kSalaryReview, PermissionDomain::Finance, AssignmentPolicy::ApprovalRequired},
    {kSalarySubmitReview, PermissionDomain::Finance, AssignmentPolicy::ApprovalRequired},
    {kSalarySupervisorReview, PermissionDomain::Finance, AssignmentPolicy::ApprovalRequired},
    {kSalaryLock, PermissionDomain::Finance, AssignmentPolicy::ApprovalRequired},
    {kSalaryProfileActivate, PermissionDomain::Finance, AssignmentPolicy::ApprovalRequired},
    {kEquityRead, PermissionDomain::Finance, AssignmentPolicy::ApprovalRequired},
    {kEquityWrite, PermissionDomain::Finance, AssignmentPolicy::ApprovalRequired},
    // medical
    {kMedicalRecordRead, PermissionDomain::Medical, AssignmentPolicy::PersonnelDirect},
    {kMedicalRecordWrite, PermissionDomain::Medical, AssignmentPolicy::PersonnelDirect},
    {kMedicalRecordFinalize, PermissionDomain::Medical, AssignmentPolicy::PersonnelDirect},
    {kMedicalRecordPrint, PermissionDomain::Medical, AssignmentPolicy::PersonnelDirect},
    {kMedicalRecordAmend, PermissionDomain::Medical, AssignmentPolicy::PersonnelDirect},
    {kMedicalRecordVoid, PermissionDomain::Medical, AssignmentPolicy::PersonnelDirect},
    {kDoctorWorkWrite, PermissionDomain::Medical, AssignmentPolicy::PersonnelDirect},
    {kScopeMedicalAssigned, PermissionDomain::Medical, AssignmentPolicy::PersonnelDirect},
    // report templates
    {kReportTemplateRead, PermissionDomain::Management, AssignmentPolicy::PersonnelDirect},
    {kReportTemplateManage, PermissionDomain::Management, AssignmentPolicy::ApprovalRequired},
    {kReportTemplatePublish, PermissionDomain::Management, AssignmentPolicy::ApprovalRequired},
    // management / audit
    {kLogsRead, PermissionDomain::Management, AssignmentPolicy::ApprovalRequired},
    {kUserDelete, PermissionDomain::Management, AssignmentPolicy::ApprovalRequired},
    {kScopeAll, PermissionDomain::Management, AssignmentPolicy::ApprovalRequired},
    {kRbacManage, PermissionDomain::Management, AssignmentPolicy::SuperAdminOnly},
    // warehouse
    {kStockRead, PermissionDomain::Warehouse, AssignmentPolicy::PersonnelDirect},
    {kStockWrite, PermissionDomain::Warehouse, AssignmentPolicy::PersonnelDirect},
    // personnel / attendance
    {kStaffRoleWrite, PermissionDomain::Personnel, AssignmentPolicy::ApprovalRequired},
    {kAttendanceRead, PermissionDomain::General, AssignmentPolicy::PersonnelDirect},
    {kAttendanceManage, PermissionDomain::Personnel, AssignmentPolicy::ApprovalRequired},
    {kEmploymentRead, PermissionDomain::Personnel, AssignmentPolicy::ApprovalRequired},
    {kEmploymentOnboard, PermissionDomain::Personnel, AssignmentPolicy::ApprovalRequired},
    {kEmploymentAssign, PermissionDomain::Personnel, AssignmentPolicy::ApprovalRequired},
    {kEmploymentRegularize, PermissionDomain::Personnel, AssignmentPolicy::ApprovalRequired},
    {kEmploymentOffboard, PermissionDomain::Personnel, AssignmentPolicy::ApprovalRequired},
    {kCompensationPropose, PermissionDomain::Personnel, AssignmentPolicy::ApprovalRequired},
    {kCompensationReassignCase, PermissionDomain::Personnel, AssignmentPolicy::ApprovalRequired},
    // management approvals
    {kEmploymentAssignmentApprove, PermissionDomain::Management, AssignmentPolicy::ApprovalRequired},
    {kCompensationApprove, PermissionDomain::Management, AssignmentPolicy::ApprovalRequired},
};

const PermissionCatalogEntry *findCatalogEntry(const std::string &key)
{
    for (const auto &entry : kCatalog)
    {
        if (key == entry.key)
        {
            return &entry;
        }
    }
    return nullptr;
}
}

const PermissionCatalogEntry *permissionCatalog()
{
    return kCatalog;
}

std::size_t permissionCatalogSize()
{
    return sizeof(kCatalog) / sizeof(kCatalog[0]);
}

std::vector<std::string> allPermissionKeys()
{
    std::vector<std::string> keys;
    keys.reserve(permissionCatalogSize());
    for (const auto &entry : kCatalog)
    {
        keys.emplace_back(entry.key);
    }
    return keys;
}

std::vector<std::string> grantablePermissionKeys()
{
    std::vector<std::string> keys;
    keys.reserve(permissionCatalogSize());
    for (const auto &entry : kCatalog)
    {
        if (std::string(entry.key) != kRbacManage)
        {
            keys.emplace_back(entry.key);
        }
    }
    return keys;
}

bool isKnownPermissionKey(const std::string &permissionKey)
{
    return findCatalogEntry(permissionKey) != nullptr;
}

bool isGrantablePermissionKey(const std::string &permissionKey)
{
    return permissionKey != kRbacManage && isKnownPermissionKey(permissionKey);
}

PermissionDomain domainOfPermission(const std::string &key)
{
    const PermissionCatalogEntry *entry = findCatalogEntry(key);
    if (!entry)
    {
        throw std::invalid_argument("Unknown permission key: " + key);
    }
    return entry->domain;
}

std::string domainKey(PermissionDomain domain)
{
    switch (domain)
    {
    case PermissionDomain::General:
        return "general";
    case PermissionDomain::Medical:
        return "medical";
    case PermissionDomain::Finance:
        return "finance";
    case PermissionDomain::Personnel:
        return "personnel";
    case PermissionDomain::Warehouse:
        return "warehouse";
    case PermissionDomain::Management:
        return "management";
    }
    return "general";
}

std::string domainChineseName(PermissionDomain domain)
{
    switch (domain)
    {
    case PermissionDomain::General:
        return "通用";
    case PermissionDomain::Medical:
        return "医疗";
    case PermissionDomain::Finance:
        return "财务";
    case PermissionDomain::Personnel:
        return "人事";
    case PermissionDomain::Warehouse:
        return "仓储";
    case PermissionDomain::Management:
        return "管理";
    }
    return "通用";
}

AssignmentPolicy minimumAssignmentPolicy(const std::string &permissionKey)
{
    const PermissionCatalogEntry *entry = findCatalogEntry(permissionKey);
    if (!entry)
    {
        // 未知 key 与目录未命中：一律 fail-closed，禁止隐式 personnel_direct。
        return AssignmentPolicy::SuperAdminOnly;
    }
    return entry->minimumPolicy;
}

AssignmentPolicy requiredAssignmentPolicy(const std::vector<std::string> &permissionKeys)
{
    AssignmentPolicy floor = AssignmentPolicy::PersonnelDirect;
    for (const std::string &key : permissionKeys)
    {
        floor = maxAssignmentPolicy(floor, minimumAssignmentPolicy(key));
    }
    return floor;
}

int assignmentPolicyRank(AssignmentPolicy policy)
{
    return static_cast<int>(policy);
}

AssignmentPolicy maxAssignmentPolicy(AssignmentPolicy a, AssignmentPolicy b)
{
    return assignmentPolicyRank(a) >= assignmentPolicyRank(b) ? a : b;
}

const char *assignmentPolicyKey(AssignmentPolicy policy)
{
    switch (policy)
    {
    case AssignmentPolicy::PersonnelDirect:
        return kAssignmentPolicyPersonnelDirect;
    case AssignmentPolicy::ApprovalRequired:
        return kAssignmentPolicyApprovalRequired;
    case AssignmentPolicy::SuperAdminOnly:
        return kAssignmentPolicySuperAdminOnly;
    }
    return kAssignmentPolicySuperAdminOnly;
}

AssignmentPolicy parseAssignmentPolicy(const std::string &value)
{
    if (value == kAssignmentPolicyPersonnelDirect)
    {
        return AssignmentPolicy::PersonnelDirect;
    }
    if (value == kAssignmentPolicyApprovalRequired)
    {
        return AssignmentPolicy::ApprovalRequired;
    }
    if (value == kAssignmentPolicySuperAdminOnly)
    {
        return AssignmentPolicy::SuperAdminOnly;
    }
    return AssignmentPolicy::SuperAdminOnly;
}

bool isValidAssignmentPolicyKey(const std::string &value)
{
    return value == kAssignmentPolicyPersonnelDirect ||
           value == kAssignmentPolicyApprovalRequired ||
           value == kAssignmentPolicySuperAdminOnly;
}
}
