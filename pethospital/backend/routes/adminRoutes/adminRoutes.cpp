#include "adminRoutes.h"
#include "../../controllers/modules/doctor/doctorHandler.h"
#include "../../controllers/modules/finance/financeHandler.h"
#include "../../controllers/modules/personnel/personnelHandler.h"
#include "../../services/logger/operationLogger.h"
#include "../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../services/auth/AuthSessionStore.h"

#include <iostream>

void adminRoutes::setupAdminRoutes(
    CrowApp &app,
    std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }

    // 用于页面首次加载、手动刷新、SSE 断线后的兜底请求。
    CROW_ROUTE(app, "/api/admins/home-data")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "获取超级管理员首页数据");
                        return;
                    }

                    adminHandler handler(dbManager);
                    crow::response response = handler.getHomeData(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "获取超级管理员首页数据", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "获取首页数据", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    // 超级管理员首页实时数据通道。
    CROW_WEBSOCKET_ROUTE(app, "/realtime/admins/home-data")
        .onaccept([dbManager](const crow::request &req, void **)
                  {
            // 前端请求传递的Token值会通过Crow框架自动解析存入req.url_params字典中。
            // req.url_params.get() 返回的是一个 C 风格字符串（const char*）
            const char *tokenParam = req.url_params.get("token");
            if (tokenParam == nullptr || std::string(tokenParam).empty())
            {
                return false;
            }

            auto claims = JwtUtils::getTokenClaims(tokenParam);
            if (!claims || claims->userId <= 0 || !dbManager || !dbManager->getSession())
            {
                return false;
            }

            // 管理端会话失效：被 bump 过的旧 token（改密码/失效后）不能继续建立实时连接。
            if (!AuthSessionStore::isSessionCurrent(claims->userId, claims->typeName, claims->sessionVersion))
            {
                return false;
            }

            std::string identifier = claims->identifier;
            return JwtUtils::isUserAuthorizedForAdminForm(
                claims->userId,
                identifier,
                claims->isEmailLogin,
                dbManager); })
        .onopen([](crow::websocket::connection &conn)
                {
            AdminHomeDataBroadcaster::instance().addConnection(&conn); })
        .onclose([](crow::websocket::connection &conn, const std::string &, uint16_t)
                 {
            AdminHomeDataBroadcaster::instance().removeConnection(&conn); })
        .onerror([](crow::websocket::connection &conn, const std::string &reason)
                 {
            std::cerr << "Admin homeData WebSocket error: " << reason << std::endl;
            AdminHomeDataBroadcaster::instance().removeConnection(&conn); });

    CROW_ROUTE(app, "/api/admins/users")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                const bool isCreate = req.method == crow::HTTPMethod::Post;
                const std::string action = isCreate ? "创建用户" : "获取用户列表";
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", action);
                        return;
                    }

                    crow::response response;
                    if (isCreate)
                    {
                        personnelHandler handler(dbManager);
                        response = handler.createUser(req);
                    }
                    else
                    {
                        adminHandler handler(dbManager);
                        response = handler.getUsers(req);
                    }
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", action, userId > 0 ? std::optional<int>(userId) : std::nullopt, isCreate);
            });

    CROW_ROUTE(app, "/api/admins/users/search")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "搜索用户");
                        return;
                    }

                    adminHandler handler(dbManager);
                    auto jsonOpt = handler.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "搜索用户", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    crow::response response = handler.searchUsers(req, jsonOpt.value());
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "搜索用户", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "搜索用户", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    CROW_ROUTE(app, "/api/admins/online-doctors/search")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "搜索在线医生");
                        return;
                    }

                    adminHandler handler(dbManager);
                    auto jsonOpt = handler.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "搜索在线医生", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    crow::response response = handler.searchOnlineDoctors(req, jsonOpt.value());
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "搜索在线医生", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "搜索在线医生", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    CROW_ROUTE(app, "/api/admins/work-time-records")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "获取工时记录");
                        return;
                    }

                    adminHandler handler(dbManager);
                    crow::response response = handler.getWorkTimeRecord(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "获取工时记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "获取工时记录", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    CROW_ROUTE(app, "/api/admins/user-deletions")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "删除用户");
                        return;
                    }

                    personnelHandler handler(dbManager);
                    crow::response response = handler.deleteUser(req, userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "删除用户", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "删除用户", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admins/doctor-work-time-changes")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "调整医生排班");
                        return;
                    }

                    adminHandler handler(dbManager);
                    auto jsonOpt = handler.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "调整医生排班", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    nlohmann::json &request_body = jsonOpt.value();
                    std::string date = request_body["date"].is_string()
                                           ? request_body["date"].get<std::string>()
                                           : request_body.dump();
                    std::string identifier = request_body["identifier"].is_string()
                                                 ? request_body["identifier"].get<std::string>()
                                                 : request_body.dump();

                    crow::response response =
                        handler.changeDoctorWorkTime(req, userId, date, identifier);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "调整医生排班", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "调整医生排班", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    // 修改医生工作状态接口
    CROW_ROUTE(app, "/api/admins/doctor-work-status-changes")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "修改医生工作状态");
                        return;
                    }

                    adminHandler handler(dbManager);
                    crow::response response = handler.handleDoctorStatusAction(req, userId, true);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "修改医生工作状态", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "修改医生工作状态", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admins/logs")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "查询操作日志");
                        return;
                    }

                    adminHandler handler(dbManager);
                    crow::response response = handler.getLogs(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "查询操作日志", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "查询操作日志", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    CROW_ROUTE(app, "/api/admins/logs/search")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "搜索操作日志");
                        return;
                    }

                    adminHandler handler(dbManager);
                    auto jsonOpt = handler.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "搜索操作日志", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    crow::response response = handler.searchLogs(req, jsonOpt.value());
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "搜索操作日志", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "搜索操作日志", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    CROW_ROUTE(app, "/api/admins/order-records")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "订单", "获取全部病历");
                    return;
                }

                adminHandler handler(dbManager);
                crow::response response = handler.getAllRecord(req, userId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "订单", "获取全部病历", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "订单", "获取全部病历", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    routes_setup = true;
}
