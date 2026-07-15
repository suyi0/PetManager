#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef SCHEDULED_TASK_MANAGER_SOURCE_PATH
#error "SCHEDULED_TASK_MANAGER_SOURCE_PATH is required"
#endif

#ifndef OPERATION_LOGGER_SOURCE_PATH
#error "OPERATION_LOGGER_SOURCE_PATH is required"
#endif

#ifndef JWT_UTILS_SOURCE_PATH
#error "JWT_UTILS_SOURCE_PATH is required"
#endif

#ifndef USER_HANDLER_AUTH_SOURCE_PATH
#error "USER_HANDLER_AUTH_SOURCE_PATH is required"
#endif

#ifndef ROLE_TYPE_UTILS_SOURCE_PATH
#error "ROLE_TYPE_UTILS_SOURCE_PATH is required"
#endif

#ifndef ADMIN_HANDLER_SOURCE_PATH
#error "ADMIN_HANDLER_SOURCE_PATH is required"
#endif

#ifndef FINANCE_HANDLER_SOURCE_PATH
#error "FINANCE_HANDLER_SOURCE_PATH is required"
#endif

#ifndef ADMIN_BROADCASTER_HEADER_PATH
#error "ADMIN_BROADCASTER_HEADER_PATH is required"
#endif

#ifndef ADMIN_BROADCASTER_SOURCE_PATH
#error "ADMIN_BROADCASTER_SOURCE_PATH is required"
#endif

#ifndef ADMIN_ROUTES_SOURCE_PATH
#error "ADMIN_ROUTES_SOURCE_PATH is required"
#endif

#ifndef FINANCE_BROADCASTER_HEADER_PATH
#error "FINANCE_BROADCASTER_HEADER_PATH is required"
#endif

#ifndef FINANCE_BROADCASTER_SOURCE_PATH
#error "FINANCE_BROADCASTER_SOURCE_PATH is required"
#endif

#ifndef FINANCE_ROUTES_SOURCE_PATH
#error "FINANCE_ROUTES_SOURCE_PATH is required"
#endif

#ifndef DOCTOR_HANDLER_SOURCE_PATH
#error "DOCTOR_HANDLER_SOURCE_PATH is required"
#endif

#ifndef PERSONNEL_HANDLER_SOURCE_PATH
#error "PERSONNEL_HANDLER_SOURCE_PATH is required"
#endif

#ifndef USER_HANDLER_SOURCE_PATH
#error "USER_HANDLER_SOURCE_PATH is required"
#endif

#ifndef USER_HANDLER_RESERVATION_SOURCE_PATH
#error "USER_HANDLER_RESERVATION_SOURCE_PATH is required"
#endif

#ifndef DATABASE_MANAGER_SOURCE_PATH
#error "DATABASE_MANAGER_SOURCE_PATH is required"
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
    if (source.find(needle) == std::string::npos)
    {
        std::cerr << "Missing expected text: " << needle << std::endl;
    }
    assert(source.find(needle) != std::string::npos);
}

void assertNotContains(const std::string &source, const std::string &needle)
{
    if (source.find(needle) != std::string::npos)
    {
        std::cerr << "Unexpected text: " << needle << std::endl;
    }
    assert(source.find(needle) == std::string::npos);
}
}

int main()
{
    const std::string scheduledTasks = readFile(SCHEDULED_TASK_MANAGER_SOURCE_PATH);
    const std::string operationLogger = readFile(OPERATION_LOGGER_SOURCE_PATH);
    const std::string jwtUtils = readFile(JWT_UTILS_SOURCE_PATH);
    const std::string userHandlerAuth = readFile(USER_HANDLER_AUTH_SOURCE_PATH);
    const std::string roleTypeUtils = readFile(ROLE_TYPE_UTILS_SOURCE_PATH);
    const std::string adminHandler = readFile(ADMIN_HANDLER_SOURCE_PATH);
    const std::string financeHandler = readFile(FINANCE_HANDLER_SOURCE_PATH);
    const std::string adminBroadcasterHeader = readFile(ADMIN_BROADCASTER_HEADER_PATH);
    const std::string adminBroadcaster = readFile(ADMIN_BROADCASTER_SOURCE_PATH);
    const std::string adminRoutes = readFile(ADMIN_ROUTES_SOURCE_PATH);
    const std::string financeBroadcasterHeader = readFile(FINANCE_BROADCASTER_HEADER_PATH);
    const std::string financeBroadcaster = readFile(FINANCE_BROADCASTER_SOURCE_PATH);
    const std::string financeRoutes = readFile(FINANCE_ROUTES_SOURCE_PATH);
    const std::string doctorHandler = readFile(DOCTOR_HANDLER_SOURCE_PATH);
    const std::string personnelHandler = readFile(PERSONNEL_HANDLER_SOURCE_PATH);
    const std::string userHandler = readFile(USER_HANDLER_SOURCE_PATH);
    const std::string userReservationHandler = readFile(USER_HANDLER_RESERVATION_SOURCE_PATH);
    const std::string databaseManager = readFile(DATABASE_MANAGER_SOURCE_PATH);

    assertContains(scheduledTasks, "u.account_type = 'staff'");
    assertNotContains(scheduledTasks, "getRoleId(dbManager, \"普通用户\")");
    assertNotContains(scheduledTasks, "u.type_id <> ?");
    assertNotContains(scheduledTasks, "FROM types");

    assertContains(operationLogger, "LEFT JOIN positions AS pos ON pos.id = u.position_id");
    assertNotContains(operationLogger, "LEFT JOIN types AS t ON u.type_id = t.id");

    assertContains(jwtUtils, "LEFT JOIN positions");
    assertContains(jwtUtils, "u.account_type = 'customer'");
    assertContains(jwtUtils, "LEFT JOIN positions AS pos ON pos.id = u.position_id");
    assertContains(jwtUtils, "COALESCE(pos.name, '')");
    assertNotContains(jwtUtils, "COALESCE(p.name, '') END AS role_name "
                                "FROM users AS u "
                                "LEFT JOIN positions AS pos");
    assertNotContains(jwtUtils, "JOIN types");
    assertNotContains(jwtUtils, "u.type_id = t.id");

    assertContains(userHandlerAuth, "LEFT JOIN positions AS pos ON pos.id = u.position_id");
    assertContains(userHandlerAuth, "COALESCE(u.position_id, 0)");
    assertNotContains(userHandlerAuth, "JOIN types");
    assertNotContains(userHandlerAuth, "u.type_id = t.id");

    assertContains(roleTypeUtils, "FROM positions");
    assertContains(roleTypeUtils, "u.account_type = 'customer'");
    assertContains(roleTypeUtils, "LEFT JOIN positions AS pos ON pos.id = u.position_id");
    assertNotContains(roleTypeUtils, "FROM types");
    assertNotContains(roleTypeUtils, "JOIN types");
    assertNotContains(roleTypeUtils, "u.type_id = t.id");

    for (const std::string *source : {
             &adminHandler,
             &financeHandler,
             &doctorHandler,
             &personnelHandler,
             &userHandler,
             &userReservationHandler,
         })
    {
        assertNotContains(*source, "JOIN types");
        assertNotContains(*source, "JOIN `types`");
        assertNotContains(*source, "FROM types");
        assertNotContains(*source, "FROM `types`");
        assertNotContains(*source, "u.type_id");
    }

    // Password changes must prove knowledge of the current password before writing a new hash.
    assertContains(userHandler, "getRequestString(request_body, \"currentPassword\"");
    assertContains(userHandler, "getRequestString(request_body, \"newPassword\"");
    assertContains(userHandler, "verify_password_hash(currentPassword, storedPassword)");
    assertContains(userHandler, "if (!currentPasswordMatches)");
    assertContains(userHandler, "AuthSessionStore::bumpSessionVersionForUser(userId)");
    assertNotContains(userHandler, "getRequestStringWithFallback(request_body, \"password\", \"newPassword\"");

    assertNotContains(financeHandler, "s.update_at");
    assertContains(financeHandler, "s.updated_at");
    assertContains(financeHandler, "buildHomeData(const crow::request &req)");
    assertContains(financeHandler, "return ResponseHelper::success(req, buildHomeData(req));");
    assertContains(financeHandler, "const nlohmann::json homeData = buildHomeData(req);");
    assertContains(financeHandler, "JOIN users AS u ON u.id = s.user_id");
    assertContains(financeHandler, "LEFT JOIN positions AS pos ON pos.id = u.position_id");
    assertContains(financeHandler, "pos.department_id");

    assertContains(adminHandler, "buildHomeData(const crow::request &req)");
    assertContains(adminHandler, "return ResponseHelper::success(req, buildHomeData(req));");
    assertContains(adminHandler, "financeData[\"userCount\"] = calculateUserCount(req);");
    assertContains(adminHandler, "financeData[\"userLogCount\"] = calculateUserLogsCount(req);");

    assertContains(adminBroadcasterHeader, "struct ConnectionContext");
    assertContains(adminBroadcasterHeader, "void addConnection(crow::websocket::connection *conn, const ConnectionContext &context)");
    assertContains(adminBroadcaster, "AuthSessionStore::isSessionCurrent(context.userId, context.sessionVersion)");
    assertContains(adminBroadcaster, "handler.buildHomeData(context.userId)");
    assertNotContains(adminBroadcaster, "{\"data\", handler.buildHomeData()}");
    assertContains(adminRoutes, "auto *context = new AdminHomeDataBroadcaster::ConnectionContext");
    assertContains(adminRoutes, "static_cast<AdminHomeDataBroadcaster::ConnectionContext *>(conn.userdata())");
    assertContains(adminRoutes, "AdminHomeDataBroadcaster::instance().addConnection(&conn, *context)");

    assertContains(financeBroadcasterHeader, "struct ConnectionContext");
    assertContains(financeBroadcasterHeader, "void addConnection(crow::websocket::connection *conn, const ConnectionContext &context)");
    assertContains(financeBroadcaster, "AuthSessionStore::isSessionCurrent(context.userId, context.sessionVersion)");
    assertContains(financeBroadcaster, "handler.buildHomeData(context.userId)");
    assertNotContains(financeBroadcaster, "{\"data\", handler.buildHomeData()}");
    assertContains(financeRoutes, "auto *context = new FinanceHomeDataBroadcaster::ConnectionContext");
    assertContains(financeRoutes, "static_cast<FinanceHomeDataBroadcaster::ConnectionContext *>(conn.userdata())");
    assertContains(financeRoutes, "FinanceHomeDataBroadcaster::instance().addConnection(&conn, *context)");

    // DB_AUTO_RUN_MIGRATIONS=true is an explicit startup contract: partial migrations
    // must abort startup instead of serving requests against a half-upgraded schema.
    assertContains(databaseManager, "if (shouldRunStartupMigrations())");
    assertContains(databaseManager, "让服务带着部分 schema 继续监听请求");
    assertContains(databaseManager, "throw;");

    return 0;
}
