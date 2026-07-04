#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef AUTH_MIDDLEWARE_SOURCE_PATH
#error "AUTH_MIDDLEWARE_SOURCE_PATH must point to authMiddleware.cpp"
#endif

#ifndef JWT_UTILS_SOURCE_PATH
#error "JWT_UTILS_SOURCE_PATH must point to jwtUtils.cpp"
#endif

namespace
{
std::string readFile(const char *path)
{
    std::ifstream input(path);
    assert(input.is_open());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}
}

int main()
{
    const std::string source = readFile(AUTH_MIDDLEWARE_SOURCE_PATH);
    const std::string jwtSource = readFile(JWT_UTILS_SOURCE_PATH);

    // Authorization must use the current DB-backed role/permission source.
    // A role stored in JWT claims can be stale after role reassignment.
    assert(source.find("RoleTypeUtils::isBossRole(claims.typeName)") == std::string::npos);
    assert(source.find("claims.typeName))\n            {\n                return true;") == std::string::npos);

    // Function-level permission checks are authorization failures, not authentication failures.
    // Missing/expired tokens remain 401, but a valid token without a required permission must be 403.
    assert(source.find("AuthorizationFailureResponse::PermissionDenied") != std::string::npos);
    assert(source.find("ResponseHelper::permission_denied(req, \"用户无权限进行此操作\"") != std::string::npos);
    assert(source.find("permissionKey); },\n                         AuthorizationFailureResponse::PermissionDenied") != std::string::npos);

    // Resource-level order scope failures should hide resource existence.
    assert(source.find("ResponseHelper::notFound(req, \"Order not found\"") != std::string::npos);
    assert(source.find("ResponseHelper::unauthorized(req, \"Access denied to this order\"") == std::string::npos);

    // Order resource authorization must use the same DataScope contract as list/search paths.
    assert(jwtSource.find("DataScope::resolveForRole(roleName, userId)") != std::string::npos);
    assert(jwtSource.find("VisibilityFilter::build(dataScope, \"o\", \"owner_id\", /*alwaysExcludeSoftDeleted=*/true)") != std::string::npos);
    assert(jwtSource.find("SELECT 1 FROM orders AS o ") != std::string::npos);
    assert(jwtSource.find("SELECT owner_id FROM orders WHERE id = ?") == std::string::npos);

    return 0;
}
