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
    // 访问权变更后的会话吊销统一走 AccessRevocation（内部 = 角色缓存失效 + 会话版本 bump）
    assertContains(adminRoutes, "AccessRevocation::revokeUserSessions(targetUserId)");
    assertContains(adminRoutes, "bumpUsersInPosition(dbManager, positionId)");
    assertContains(adminRoutes, "replaceUserScopes(dbManager, userId, targetUserId");
    assertContains(adminRoutes, "department_id");
    assertContains(adminRoutes, "branch_id");
    assertContains(adminRoutes, "targetPositionHasPermissions");
    assertContains(adminRoutes, "分配带权限的岗位需要权限管理权限");
    assertContains(adminRoutes, "userId = isValidManagementToken(req, res, dbManager);");
    assertContains(adminRoutes, "AccessRevocation::closeRealtimeConnections()");
    assertNotContains(adminRoutes, "Permissions::allPermissionKeys()");

    return 0;
}
