#include "financeRoutes.h"
#include "../../services/logger/operationLogger.h"
#include "../../services/realtime/financeBroadcaster/financeHomeDataBroadcaster.h"
#include "../../services/auth/AuthSessionStore.h"
#include "../../utils/permissions/Permissions.h"

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
                    userId = isValidFinancePortalToken(req, res, dbManager);
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
    CROW_WEBSOCKET_ROUTE(app, "/realtime/finance/home-data")
        .onaccept([dbManager](const crow::request &req, void **userdata)
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

            // 管理端会话失效：被 bump 过的旧 token（改密码/失效后）不能继续建立实时连接。
            if (!AuthSessionStore::isSessionCurrent(claims->userId, claims->sessionVersion))
            {
                return false;
            }

            std::string identifier = claims->identifier;
            if (!JwtUtils::isUserAuthorizedForFinancePortal(
                    claims->userId,
                    identifier,
                    claims->isEmailLogin,
                    dbManager))
            {
                return false;
            }

            auto *context = new FinanceHomeDataBroadcaster::ConnectionContext{
                claims->userId,
                claims->sessionVersion};
            *userdata = context;
            return true; })
        .onopen([](crow::websocket::connection &conn)
                {
            auto *context = static_cast<FinanceHomeDataBroadcaster::ConnectionContext *>(conn.userdata());
            if (!context || context->userId <= 0)
            {
                conn.close("invalid_finance_home_session");
                return;
            }

            FinanceHomeDataBroadcaster::instance().addConnection(&conn, *context);
            delete context;
            conn.userdata(nullptr); })
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
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalaryWrite);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "添加或更新员工工资");
                        return;
                    }

                    financeHandler handler(dbManager);
                    crow::response response = handler.savePayrollEmployee(req, goalUserId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "添加或更新员工工资", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "添加或更新员工工资", Permissions::kSalaryWrite, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/employees/<int>/salary-profile")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int goalUserId)
            {
                // GET 保留 salary:read；POST 硬切退役，不再走 saveSalaryProfile 写金额。
                const bool isWrite = req.method == crow::HTTPMethod::Post;
                const std::string permission = Permissions::kSalaryRead;
                const std::string action = isWrite ? "保存员工薪资配置(已退役)" : "获取员工薪资配置";
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, permission);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", action);
                        return;
                    }
                    financeHandler handler(dbManager);
                    crow::response response = isWrite
                                                  ? handler.saveSalaryProfile(req, goalUserId)
                                                  : handler.getSalaryProfile(req, goalUserId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", action, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", action, permission, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    // v6 财务激活：精确 salary-profile:activate
    CROW_ROUTE(app, "/api/finance/compensation-activations")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(
                        req, res, dbManager, Permissions::kSalaryProfileActivate);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(
                            dbManager, req, res, "财务", "薪酬激活列表");
                        return;
                    }
                    financeHandler handler(dbManager);
                    crow::response response = handler.listCompensationActivations(req, userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(
                        dbManager, req, "财务", "薪酬激活列表", e.what(),
                        userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(
                    dbManager, req, res, "财务", "薪酬激活列表",
                    Permissions::kSalaryProfileActivate,
                    userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/compensation-activations/<int>/confirm")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int proposalId)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(
                        req, res, dbManager, Permissions::kSalaryProfileActivate);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(
                            dbManager, req, res, "财务", "确认薪酬激活");
                        return;
                    }
                    BaseHandler parser(dbManager);
                    auto jsonOpt = parser.parseJson(req, res);
                    if (jsonOpt)
                    {
                        financeHandler handler(dbManager);
                        crow::response response = handler.confirmCompensationActivation(
                            req, userId, static_cast<long long>(proposalId), *jsonOpt);
                        ProcessHandlerResponse(req, res, response);
                    }
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(
                        dbManager, req, "财务", "确认薪酬激活", e.what(),
                        userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(
                    dbManager, req, res, "财务", "确认薪酬激活",
                    Permissions::kSalaryProfileActivate,
                    userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/compensation-activations/<int>/return")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int proposalId)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(
                        req, res, dbManager, Permissions::kSalaryProfileActivate);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(
                            dbManager, req, res, "财务", "退回薪酬激活");
                        return;
                    }
                    BaseHandler parser(dbManager);
                    auto jsonOpt = parser.parseJson(req, res);
                    if (jsonOpt)
                    {
                        financeHandler handler(dbManager);
                        crow::response response = handler.returnCompensationActivation(
                            req, userId, static_cast<long long>(proposalId), *jsonOpt);
                        ProcessHandlerResponse(req, res, response);
                    }
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(
                        dbManager, req, "财务", "退回薪酬激活", e.what(),
                        userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(
                    dbManager, req, res, "财务", "退回薪酬激活",
                    Permissions::kSalaryProfileActivate,
                    userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/payroll-periods/current/employees/<int>/first-review")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int goalUserId)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalaryReview);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "员工工资初审");
                        return;
                    }
                    financeHandler handler(dbManager);
                    crow::response response = handler.reviewPayrollEmployee(req, goalUserId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "员工工资初审", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "员工工资初审", Permissions::kSalaryReview, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    // 获取员工工资列表摘要路由
    CROW_ROUTE(app, "/api/finance/salary-summaries/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int page)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalaryRead);
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
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "获取工资管理数据", Permissions::kSalaryRead, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/salary-employees/search")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalaryRead);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "搜索员工工资");
                        return;
                    }

                    financeHandler handler(dbManager);
                    auto jsonOpt = handler.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "搜索员工工资", Permissions::kSalaryRead, userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    crow::response response = handler.getPayrollEmployees(req, jsonOpt.value());
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "搜索员工工资", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "搜索员工工资", Permissions::kSalaryRead, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/payroll-periods/current/submit-review")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalarySubmitReview);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "提交工资主管复审");
                        return;
                    }
                    financeHandler handler(dbManager);
                    crow::response response = handler.submitPayrollReview(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "提交工资主管复审", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "提交工资主管复审", Permissions::kSalarySubmitReview, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/payroll-periods/current/supervisor-review")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalarySupervisorReview);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "主管工资复审");
                        return;
                    }
                    financeHandler handler(dbManager);
                    crow::response response = handler.supervisorReviewPayroll(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "主管工资复审", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "主管工资复审", Permissions::kSalarySupervisorReview, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/payroll-periods/current/lock")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalaryLock);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "锁定工资周期");
                        return;
                    }
                    financeHandler handler(dbManager);
                    crow::response response = handler.lockPayroll(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "锁定工资周期", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "锁定工资周期", Permissions::kSalaryLock, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/payroll-periods/current/revisions")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalaryWrite);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "创建工资修订版");
                        return;
                    }
                    financeHandler handler(dbManager);
                    crow::response response = handler.createPayrollRevision(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "创建工资修订版", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "创建工资修订版", Permissions::kSalaryWrite, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/payroll-periods/current/audit-events")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalaryRead);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "财务", "查看工资周期审计");
                        return;
                    }
                    financeHandler handler(dbManager);
                    crow::response response = handler.getPayrollAuditEvents(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "查看工资周期审计", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "查看工资周期审计", Permissions::kSalaryRead, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    // 获取员工工资详情路由
    CROW_ROUTE(app, "/api/finance/salary-records/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int salaryId)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalaryRead);
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

                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "财务", "获取工资详情", Permissions::kSalaryRead, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/finance/salary-records/<int>/change-history")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int salaryId)
            {
                ResponseEnder ender{res};
                const int userId = isValidPermissionToken(req, res, dbManager, Permissions::kSalaryRead);
                if (res.code != 200 || userId == -1) return;
                financeHandler handler(dbManager);
                crow::response response = handler.getSalaryChangeHistory(req, salaryId);
                ProcessHandlerResponse(req, res, response);
            });

    CROW_ROUTE(app, "/api/finance/expenses")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidFinancePortalToken(req, res, dbManager);
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
