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

std::vector<std::string> allPermissionKeys();
std::vector<std::string> grantablePermissionKeys();
PermissionDomain domainOfPermission(const std::string &permissionKey);
std::string domainKey(PermissionDomain domain);
std::string domainChineseName(PermissionDomain domain);

// 检测权限键是否已知（在 kAllPermissionKeys 中），用于判定前端传入的权限键是否合法。
bool isKnownPermissionKey(const std::string &permissionKey);

// 检测权限键是否可授予（grantable），用于前端权限管理表单的选项过滤。
bool isGrantablePermissionKey(const std::string &permissionKey);
}
