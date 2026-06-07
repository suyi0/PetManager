#include "adminRoutes.h"
#include "../../controllers/modules/doctor/doctorHandler.h"
#include "../../controllers/modules/finance/financeHandler.h"
#include "../../controllers/modules/personnel/personnelHandler.h"
#include "../../services/logger/operationLogger.h"

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
    CROW_ROUTE(app, "/api/admin/getHomeData")
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

    CROW_ROUTE(app, "/api/admin/getUsers")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "获取用户列表");
                        return;
                    }

                    adminHandler handler(dbManager);
                    crow::response response = handler.getUsers(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "获取用户列表", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "获取用户列表", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    CROW_ROUTE(app, "/api/admin/getWorkTimeRecord")
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

    CROW_ROUTE(app, "/api/admin/createUser")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "管理", "创建用户");
                        return;
                    }

                    personnelHandler handler(dbManager);
                    crow::response response = handler.createUser(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "管理", "创建用户", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "管理", "创建用户", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/admin/deleteUser")
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

    CROW_ROUTE(app, "/api/admin/changeDoctorWorkTime")
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
    CROW_ROUTE(app, "/api/admin/changeDoctorWorkStatus")
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

    CROW_ROUTE(app, "/api/admin/getLogs")
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

    CROW_ROUTE(app, "/api/admin/order/getAllRecord")
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
