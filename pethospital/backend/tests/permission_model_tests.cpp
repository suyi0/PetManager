#include "../utils/permissions/Permissions.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <string>
#include <vector>

// 权限目录契约：
// 1. 单一权威 catalog：key/domain/minimum_policy 显式声明
// 2. allPermissionKeys / isKnown / domain / minimum 均由 catalog 派生
// 3. 未知 key fail-closed SuperAdminOnly；禁止已知 key 隐式 personnel_direct 兜底
// 4. 元权限 rbac:manage：known 但不可授予，下限 SuperAdminOnly
namespace
{
bool contains(const std::vector<std::string> &items, const std::string &value)
{
    return std::find(items.begin(), items.end(), value) != items.end();
}
}

int main()
{
    const std::vector<std::string> all = Permissions::allPermissionKeys();
    const std::vector<std::string> grantable = Permissions::grantablePermissionKeys();

    // catalog size == all keys
    assert(all.size() == Permissions::permissionCatalogSize());
    assert(Permissions::permissionCatalogSize() > 0);

    // 每个 catalog 条目：key 非空、isKnown、domain 可解析、minimum 显式（枚举合法）
    std::set<std::string> seen;
    for (std::size_t i = 0; i < Permissions::permissionCatalogSize(); ++i)
    {
        const auto &entry = Permissions::permissionCatalog()[i];
        assert(entry.key != nullptr);
        assert(entry.key[0] != '\0');
        const std::string key(entry.key);
        assert(seen.insert(key).second); // 无重复
        assert(Permissions::isKnownPermissionKey(key));
        assert(Permissions::domainOfPermission(key) == entry.domain);
        assert(Permissions::minimumAssignmentPolicy(key) == entry.minimumPolicy);
        assert(Permissions::assignmentPolicyRank(entry.minimumPolicy) >= 0);
        assert(Permissions::assignmentPolicyRank(entry.minimumPolicy) <= 2);
        assert(!Permissions::domainKey(entry.domain).empty());
        assert(!Permissions::domainChineseName(entry.domain).empty());
    }

    // 期望关键 key 均在目录
    const std::vector<std::string> expectedKeys = {
        Permissions::kPortalBoss,
        Permissions::kPortalFinance,
        Permissions::kPortalSuperAdmin,
        Permissions::kPortalPersonnel,
        Permissions::kPortalMedical,
        Permissions::kPortalWarehouse,
        Permissions::kPortalUser,
        Permissions::kSalaryRead,
        Permissions::kSalaryWrite,
        Permissions::kSalaryReview,
        Permissions::kSalarySubmitReview,
        Permissions::kSalarySupervisorReview,
        Permissions::kSalaryLock,
        Permissions::kSalaryProfileActivate,
        Permissions::kLogsRead,
        Permissions::kMedicalRecordRead,
        Permissions::kMedicalRecordWrite,
        Permissions::kMedicalRecordFinalize,
        Permissions::kMedicalRecordPrint,
        Permissions::kMedicalRecordAmend,
        Permissions::kMedicalRecordVoid,
        Permissions::kReportTemplateRead,
        Permissions::kReportTemplateManage,
        Permissions::kReportTemplatePublish,
        Permissions::kDoctorWorkWrite,
        Permissions::kUserDelete,
        Permissions::kEquityRead,
        Permissions::kEquityWrite,
        Permissions::kStockRead,
        Permissions::kStockWrite,
        Permissions::kStaffRoleWrite,
        Permissions::kAttendanceRead,
        Permissions::kAttendanceManage,
        Permissions::kScopeAll,
        Permissions::kScopeMedicalAssigned,
        Permissions::kRbacManage,
        Permissions::kEmploymentRead,
        Permissions::kEmploymentOnboard,
        Permissions::kEmploymentAssign,
        Permissions::kEmploymentRegularize,
        Permissions::kEmploymentOffboard,
        Permissions::kCompensationPropose,
        Permissions::kCompensationReassignCase,
        Permissions::kEmploymentAssignmentApprove,
        Permissions::kCompensationApprove,
    };
    for (const std::string &key : expectedKeys)
    {
        assert(contains(all, key));
        assert(Permissions::isKnownPermissionKey(key));
    }

    // 显式下限抽样（禁止依赖默认 direct）
    assert(Permissions::minimumAssignmentPolicy(Permissions::kRbacManage) ==
           Permissions::AssignmentPolicy::SuperAdminOnly);
    assert(Permissions::minimumAssignmentPolicy(Permissions::kPortalBoss) ==
           Permissions::AssignmentPolicy::ApprovalRequired);
    assert(Permissions::minimumAssignmentPolicy(Permissions::kEmploymentOnboard) ==
           Permissions::AssignmentPolicy::ApprovalRequired);
    assert(Permissions::minimumAssignmentPolicy(Permissions::kSalaryProfileActivate) ==
           Permissions::AssignmentPolicy::ApprovalRequired);
    assert(Permissions::minimumAssignmentPolicy(Permissions::kPortalMedical) ==
           Permissions::AssignmentPolicy::PersonnelDirect);
    assert(Permissions::minimumAssignmentPolicy(Permissions::kStockWrite) ==
           Permissions::AssignmentPolicy::PersonnelDirect);

    // 未知 key fail-closed
    assert(!Permissions::isKnownPermissionKey(""));
    assert(!Permissions::isKnownPermissionKey("portal:hacker"));
    assert(!Permissions::isGrantablePermissionKey("portal:hacker"));
    assert(Permissions::minimumAssignmentPolicy("portal:hacker") ==
           Permissions::AssignmentPolicy::SuperAdminOnly);
    assert(Permissions::minimumAssignmentPolicy("totally-unknown-key") ==
           Permissions::AssignmentPolicy::SuperAdminOnly);

    // requiredAssignmentPolicy：未知键抬升整组
    assert(Permissions::requiredAssignmentPolicy({"portal:medical"}) ==
           Permissions::AssignmentPolicy::PersonnelDirect);
    assert(Permissions::requiredAssignmentPolicy({"portal:medical", "portal:hacker"}) ==
           Permissions::AssignmentPolicy::SuperAdminOnly);
    assert(Permissions::requiredAssignmentPolicy({"portal:medical", "portal:boss"}) ==
           Permissions::AssignmentPolicy::ApprovalRequired);

    // 元权限不可委派
    assert(!contains(grantable, Permissions::kRbacManage));
    assert(!Permissions::isGrantablePermissionKey(Permissions::kRbacManage));
    assert(grantable.size() == all.size() - 1);

    // domain 抽样
    assert(Permissions::domainOfPermission(Permissions::kEmploymentRead) ==
           Permissions::PermissionDomain::Personnel);
    assert(Permissions::domainOfPermission(Permissions::kEmploymentAssignmentApprove) ==
           Permissions::PermissionDomain::Management);
    assert(Permissions::domainOfPermission(Permissions::kSalaryProfileActivate) ==
           Permissions::PermissionDomain::Finance);

    return 0;
}
