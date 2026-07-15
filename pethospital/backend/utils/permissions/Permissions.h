#pragma once

#include <string>
#include <vector>

// 定义权限键值
namespace Permissions
{
// 权限域模型（authoritative）：每个已知权限键唯一归属 general、medical、finance、
// personnel、warehouse、management 之一；职位与个人授权都必须受目标职位允许域约束。
enum class PermissionDomain
{
    General,
    Medical,
    Finance,
    Personnel,
    Warehouse,
    Management,
};

// 职位派岗策略（authoritative）：人事可直接派岗 / 需管理审批 / 仅超管。
// 权限目录为每个 key 显式声明 minimum_assignment_policy；未知 key fail-closed 为 SuperAdminOnly。
enum class AssignmentPolicy
{
    PersonnelDirect = 0,
    ApprovalRequired = 1,
    SuperAdminOnly = 2,
};

// 目录条目：key / domain / minimum_assignment_policy 三位一体，单一权威源。
struct PermissionCatalogEntry
{
    const char *key;
    PermissionDomain domain;
    AssignmentPolicy minimumPolicy;
};

inline constexpr const char *kPortalBoss = "portal:boss";
inline constexpr const char *kPortalFinance = "portal:finance";
inline constexpr const char *kPortalSuperAdmin = "portal:super-admin";
inline constexpr const char *kPortalPersonnel = "portal:personnel";
inline constexpr const char *kPortalMedical = "portal:medical";
inline constexpr const char *kPortalWarehouse = "portal:warehouse";
inline constexpr const char *kPortalUser = "portal:user";

inline constexpr const char *kSalaryRead = "salary:read";
inline constexpr const char *kSalaryWrite = "salary:write";
inline constexpr const char *kSalaryReview = "salary:review";
inline constexpr const char *kSalarySubmitReview = "salary:submit-review";
inline constexpr const char *kSalarySupervisorReview = "salary:supervisor-review";
inline constexpr const char *kSalaryLock = "salary:lock";
inline constexpr const char *kSalaryProfileActivate = "salary-profile:activate";
inline constexpr const char *kLogsRead = "logs:read";
inline constexpr const char *kMedicalRecordRead = "medical-record:read";
inline constexpr const char *kMedicalRecordWrite = "medical-record:write";
inline constexpr const char *kMedicalRecordFinalize = "medical-record:finalize";
inline constexpr const char *kMedicalRecordPrint = "medical-record:print";
inline constexpr const char *kMedicalRecordAmend = "medical-record:amend";
inline constexpr const char *kMedicalRecordVoid = "medical-record:void";
inline constexpr const char *kReportTemplateRead = "report-template:read";
inline constexpr const char *kReportTemplateManage = "report-template:manage";
inline constexpr const char *kReportTemplatePublish = "report-template:publish";
inline constexpr const char *kDoctorWorkWrite = "doctor-work:write";
inline constexpr const char *kUserDelete = "user:delete";
inline constexpr const char *kEquityRead = "equity:read";
inline constexpr const char *kEquityWrite = "equity:write";
inline constexpr const char *kStockRead = "stock:read";
inline constexpr const char *kStockWrite = "stock:write";
inline constexpr const char *kStaffRoleWrite = "staff-role:write";
inline constexpr const char *kAttendanceRead = "attendance:read";
inline constexpr const char *kAttendanceManage = "attendance:manage";
inline constexpr const char *kScopeAll = "scope:all";
inline constexpr const char *kScopeMedicalAssigned = "scope:medical-assigned";
inline constexpr const char *kRbacManage = "rbac:manage";

inline constexpr const char *kEmploymentRead = "employment:read";
inline constexpr const char *kEmploymentOnboard = "employment:onboard";
inline constexpr const char *kEmploymentAssign = "employment:assign";
inline constexpr const char *kEmploymentRegularize = "employment:regularize";
inline constexpr const char *kEmploymentOffboard = "employment:offboard";
inline constexpr const char *kCompensationPropose = "compensation:propose";
inline constexpr const char *kCompensationReassignCase = "compensation:reassign-case";
inline constexpr const char *kEmploymentAssignmentApprove = "employment-assignment:approve";
inline constexpr const char *kCompensationApprove = "compensation:approve";

inline constexpr const char *kAssignmentPolicyPersonnelDirect = "personnel_direct";
inline constexpr const char *kAssignmentPolicyApprovalRequired = "approval_required";
inline constexpr const char *kAssignmentPolicySuperAdminOnly = "super_admin_only";

// 权威目录：allPermissionKeys / isKnown / domain / minimum 均由此派生。
const PermissionCatalogEntry *permissionCatalog();
std::size_t permissionCatalogSize();

std::vector<std::string> allPermissionKeys();
std::vector<std::string> grantablePermissionKeys();
PermissionDomain domainOfPermission(const std::string &permissionKey);
std::string domainKey(PermissionDomain domain);
std::string domainChineseName(PermissionDomain domain);

bool isKnownPermissionKey(const std::string &permissionKey);
bool isGrantablePermissionKey(const std::string &permissionKey);

// 未知 key → SuperAdminOnly。已知 key 必须在目录中显式声明，无隐式 direct 兜底。
AssignmentPolicy minimumAssignmentPolicy(const std::string &permissionKey);

// 一组权限的综合安全下限：取各 key 下限最大值；任一未知 key → SuperAdminOnly。
AssignmentPolicy requiredAssignmentPolicy(const std::vector<std::string> &permissionKeys);

int assignmentPolicyRank(AssignmentPolicy policy);
AssignmentPolicy maxAssignmentPolicy(AssignmentPolicy a, AssignmentPolicy b);
const char *assignmentPolicyKey(AssignmentPolicy policy);
AssignmentPolicy parseAssignmentPolicy(const std::string &value);
bool isValidAssignmentPolicyKey(const std::string &value);
}
