#include "financeRoutes.h"
#include "../../services/logger/operationLogger.h"
#include "../../services/realtime/financeBroadcaster/financeHomeDataBroadcaster.h"

#include <iostream>

void financeRoutes::setupFinanceRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }

    // 获取首页数据路由
    CROW_ROUTE(app, "/api/finance/home-data")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 && userId == -1)
                    {
                        OperationLogger::LogAuthorizationFailure(dbManager, req, res, "财务", "获取首页数据");
                        return;
                    }

                    financeHandler handler(dbManager);
                    crow::response response = handler.getHomeData(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "获取首页数据", "route exception", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "财务", "获取首页数据", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    // 财务端首页实时数据通道。
    CROW_WEBSOCKET_ROUTE(app, "/ws/finance/home-data")
        .onaccept([dbManager](const crow::request &req, void **)
                  {
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

            std::string identifier = claims->identifier;
            return JwtUtils::isUserAuthorizedForAdminForm(
                claims->userId,
                identifier,
                claims->isEmailLogin,
                dbManager); })
        .onopen([](crow::websocket::connection &conn)
                {
            FinanceHomeDataBroadcaster::instance().addConnection(&conn); })
        .onclose([](crow::websocket::connection &conn, const std::string &, uint16_t)
                 {
            FinanceHomeDataBroadcaster::instance().removeConnection(&conn); })
        .onerror([](crow::websocket::connection &conn, const std::string &reason)
                 {
            std::cerr << "Finance homeData WebSocket error: " << reason << std::endl;
            FinanceHomeDataBroadcaster::instance().removeConnection(&conn); });

    // 添加或更新员工工资路由
    CROW_ROUTE(app, "/api/finance/employee-salaries/<int>")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int goalUserId)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "添加或更新员工工资");
                        return;
                    }

                    financeHandler handler(dbManager);
                    crow::response response = handler.updateEmployeeSalary(req, goalUserId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "添加或更新员工工资", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "财务", "添加或更新员工工资", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    // 获取员工工资列表摘要路由
    CROW_ROUTE(app, "/api/finance/salary-summaries/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int page)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "获取工资管理数据");
                        return;
                    }

                    financeHandler handler(dbManager);
                    crow::response response = handler.getSalarySummary(req, page);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "获取工资管理数据", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "财务", "获取工资管理数据", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    // 获取员工工资详情路由
    CROW_ROUTE(app, "/api/finance/salary-records/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int salaryId)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "获取工资详情");
                        return;
                    }

                    financeHandler handler(dbManager);
                    crow::response response = handler.getSalaryInformation(req, salaryId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "获取工资详情", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }

                OperationLogger::FinishLoggedRoute(dbManager, req, res, "财务", "获取工资详情", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/expenses")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "获取开支数据");
                        return;
                    }

                    financeHandler handler(dbManager);
                    crow::response response = handler.getExpenseData(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "获取开支数据", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "财务", "获取开支数据", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            });

    routes_setup = true;
}
