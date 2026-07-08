#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef RBAC_SERVICE_SOURCE_PATH
#error "RBAC_SERVICE_SOURCE_PATH is required"
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
    const std::string source = readFile(RBAC_SERVICE_SOURCE_PATH);

    assertContains(source, "Permissions::isKnownPermissionKey(permissionKey)");
    assertContains(source, "Permissions::isGrantablePermissionKey(permissionKey)");
    assertContains(source, "FROM position_permissions");
    assertContains(source, "JOIN positions AS p ON p.id = u.position_id");
    assertContains(source, "systemKey == \"super-admin\"");
    assertContains(source, "permissionKey == Permissions::kRbacManage");
    assertNotContains(source, "UserRoleCache");
    assertNotContains(source, "Redis");

    return 0;
}
