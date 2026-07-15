#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef ADMIN_ROUTES_SOURCE_PATH
#error "ADMIN_ROUTES_SOURCE_PATH is required"
#endif

namespace
{
std::string readFile(const char *path)
{
    std::ifstream file(path);
    assert(file.is_open());
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void assertContains(const std::string &source, const std::string &needle)
{
    assert(source.find(needle) != std::string::npos);
}

void assertNotContains(const std::string &source, const std::string &needle)
{
    assert(source.find(needle) == std::string::npos);
}
}

int main()
{
    const std::string adminRoutes = readFile(ADMIN_ROUTES_SOURCE_PATH);

    assertContains(adminRoutes, "\"/api/admin/rbac/permissions/catalog\"");
    assertContains(adminRoutes, "\"/api/admin/org/departments\"");
    assertContains(adminRoutes, "\"/api/admin/org/positions\"");
    assertContains(adminRoutes, "\"/api/admin/rbac/positions/<int>/permissions\"");
    assertContains(adminRoutes, "\"/api/admin/rbac/positions/<int>/apply-template\"");
    assertContains(adminRoutes, "\"/api/admin/rbac/permission-templates\"");
    assertContains(adminRoutes, "\"/api/admin/users/<int>/position\"");
    assertContains(adminRoutes, "\"/api/admin/users/<int>/scopes\"");
    assertContains(adminRoutes, "Permissions::grantablePermissionKeys()");
    assertContains(adminRoutes, "Permissions::kRbacManage");
    assertContains(adminRoutes, "isValidManagementToken(req, res, dbManager)");
    assertContains(adminRoutes, "isValidPermissionToken(req, res, dbManager, Permissions::kRbacManage)");
    assertContains(adminRoutes, "Permissions::isGrantablePermissionKey(permissionKey)");
    assertContains(adminRoutes, "permissionKey == Permissions::kRbacManage");
    assertContains(adminRoutes, "system_key = 'super-admin'");
    assertContains(adminRoutes, "bumpUsersInPosition(dbManager, positionId)");
    assertContains(adminRoutes, "replaceUserScopes(dbManager, userId, targetUserId");
    assertContains(adminRoutes, "department_id");
    assertContains(adminRoutes, "branch_id");
    assertContains(adminRoutes, "userId = isValidManagementToken(req, res, dbManager);");
    assertContains(adminRoutes, "AccessRevocation::closeRealtimeConnections()");
    assertNotContains(adminRoutes, "Permissions::allPermissionKeys()");

    // 任职写入口统一服务（禁止平行 UPDATE users.position_id）
    assertContains(adminRoutes, "EmploymentAssignmentService::assign");
    assertContains(adminRoutes, "PositionPermissionService::replacePermissions");
    assertContains(adminRoutes, "expected_current_position_id");
    assertContains(adminRoutes, "assignment_policy");
    assertNotContains(adminRoutes, "UPDATE users SET account_type = 'staff', position_id = ? WHERE id = ?");
    assertNotContains(adminRoutes, "UPDATE users SET account_type = 'customer', position_id = NULL WHERE id = ?");

    // B15: admin 派岗 reason 必填，无默认兜底文本
    assertContains(adminRoutes, "reason 不能为空");
    assertNotContains(adminRoutes, "Admin position assignment");

    return 0;
}
