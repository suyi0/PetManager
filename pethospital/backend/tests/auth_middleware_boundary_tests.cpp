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

#ifndef DOCTOR_ROUTES_SOURCE_PATH
#error "DOCTOR_ROUTES_SOURCE_PATH must point to doctorRoutes.cpp"
#endif

#ifndef ADMIN_HANDLER_SOURCE_PATH
#error "ADMIN_HANDLER_SOURCE_PATH must point to adminHandler.cpp"
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

std::string sectionBetween(const std::string &source, const std::string &startNeedle, const std::string &endNeedle)
{
    const std::size_t start = source.find(startNeedle);
    assert(start != std::string::npos);

    const std::size_t end = source.find(endNeedle, start + startNeedle.size());
    assert(end != std::string::npos);

    return source.substr(start, end - start);
}
}

int main()
{
    const std::string source = readFile(AUTH_MIDDLEWARE_SOURCE_PATH);
    const std::string jwtSource = readFile(JWT_UTILS_SOURCE_PATH);
    const std::string doctorRoutesSource = readFile(DOCTOR_ROUTES_SOURCE_PATH);
    const std::string adminHandlerSource = readFile(ADMIN_HANDLER_SOURCE_PATH);

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

    const std::string doctorOrderInformationRoute = sectionBetween(
        doctorRoutesSource,
        "CROW_ROUTE(app, \"/api/doctors/orders/<int>/information\")",
        "// 医生端管理指定用户的宠物档案路由");
    assert(doctorOrderInformationRoute.find("isValidMedicalStaffToken(req, res, dbManager)") != std::string::npos);
    assert(doctorOrderInformationRoute.find("JwtUtils::isUserAuthorizedForOrder(userId, orderId, dbManager)") != std::string::npos);
    assert(doctorOrderInformationRoute.find("ResponseHelper::notFound(req, \"Order not found\")") != std::string::npos);

    const std::string adminOrderRecordsHandler = sectionBetween(
        adminHandlerSource,
        "crow::response adminHandler::getAllRecord",
        "return ResponseHelper::success(req, response_data);");
    assert(adminOrderRecordsHandler.find("RoleTypeUtils::getUserRoleName(dbManager, userId)") != std::string::npos);
    assert(adminOrderRecordsHandler.find("DataScope::resolveForRole(roleName, userId)") != std::string::npos);
    assert(adminOrderRecordsHandler.find("VisibilityFilter::build(dataScope, \"o\", \"owner_id\", /*alwaysExcludeSoftDeleted=*/true)") != std::string::npos);
    assert(adminOrderRecordsHandler.find("filter.whereSql") != std::string::npos);
    assert(adminOrderRecordsHandler.find("if (filter.bindsUserId)") != std::string::npos);
    assert(adminOrderRecordsHandler.find("query.bind(userId)") != std::string::npos);
    assert(adminOrderRecordsHandler.find("WHERE p.user_id = ?") == std::string::npos);

    return 0;
}
