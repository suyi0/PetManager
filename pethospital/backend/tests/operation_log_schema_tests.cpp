#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef DATABASE_MIGRATIONS_SOURCE_PATH
#error "DATABASE_MIGRATIONS_SOURCE_PATH is required"
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
    const std::string migrations = readFile(DATABASE_MIGRATIONS_SOURCE_PATH);

    assertContains(migrations, "system_role VARCHAR(64) NULL");
    assertContains(migrations, "user_role VARCHAR(64) NULL");
    assertNotContains(migrations, "system_role ENUM(");
    assertNotContains(migrations, "user_role ENUM(");
    assertNotContains(migrations, "kOperationRoleEnum");
    assertNotContains(migrations, "alignOperationRoleEnum");

    return 0;
}
