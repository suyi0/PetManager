#include "bossRoutes.h"
#include "../../services/logger/operationLogger.h"
#include "../../utils/permissions/Permissions.h"
#include "../../utils/Utils.h"

void bossRoutes::setupBossRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }

    CROW_ROUTE(app, "/api/bosses/total-stock-allocations")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kEquityWrite);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "boss", "分配股份总额");
                        return;
                    }

                    bossHandler handler(dbManager);
                    crow::response response = handler.allocateTotalStock(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "boss", "分配股份总额", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "boss", "分配股份总额", Permissions::kEquityWrite, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/bosses/stock-allocations")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kEquityWrite);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "boss", "分配个人股份份额");
                        return;
                    }

                    bossHandler handler(dbManager);
                    crow::response response = handler.allocateStock(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "boss", "分配个人股份份额", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "boss", "分配个人股份份额", Permissions::kEquityWrite, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/bosses/stock-changes")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kEquityWrite);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "boss", "修改个人股份份额");
                        return;
                    }

                    bossHandler handler(dbManager);
                    crow::response response = handler.changeStock(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "boss", "修改个人股份份额", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "boss", "修改个人股份份额", Permissions::kEquityWrite, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/bosses/stocks")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(req, res, dbManager, Permissions::kEquityRead);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "boss", "获取公司股份分布");
                        return;
                    }

                    bossHandler handler(dbManager);
                    crow::response response = handler.getStock(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "boss", "获取公司股份分布", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "boss", "获取公司股份分布", Permissions::kEquityRead, userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    // v6: 复用复数 bosses 模块 + employment-assignment-approvals；精确 employment-assignment:approve。
    CROW_ROUTE(app, "/api/bosses/employment-assignment-approvals")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(
                        req, res, dbManager, Permissions::kEmploymentAssignmentApprove);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(
                            dbManager, req, res, "boss", "任职审批列表");
                        return;
                    }

                    bossHandler handler(dbManager);
                    crow::response response = handler.listEmploymentAssignmentApprovals(req, userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(
                        dbManager, req, "boss", "任职审批列表", e.what(),
                        userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(
                    dbManager, req, res, "boss", "任职审批列表",
                    Permissions::kEmploymentAssignmentApprove,
                    userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/bosses/employment-assignment-approvals/<int>/decision")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int requestId)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(
                        req, res, dbManager, Permissions::kEmploymentAssignmentApprove);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(
                            dbManager, req, res, "boss", "任职审批决策");
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto jsonOpt = parser.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        // 由 FinishSensitiveRoute 收尾
                    }
                    else
                    {
                        bossHandler handler(dbManager);
                        crow::response response = handler.decideEmploymentAssignmentApproval(
                            req, userId, static_cast<long long>(requestId), *jsonOpt);
                        ProcessHandlerResponse(req, res, response);
                    }
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(
                        dbManager, req, "boss", "任职审批决策", e.what(),
                        userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(
                    dbManager, req, res, "boss", "任职审批决策",
                    Permissions::kEmploymentAssignmentApprove,
                    userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    // v6 薪酬管理审批：精确 compensation:approve
    CROW_ROUTE(app, "/api/bosses/compensation-approvals")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(
                        req, res, dbManager, Permissions::kCompensationApprove);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(
                            dbManager, req, res, "boss", "薪酬审批列表");
                        return;
                    }

                    bossHandler handler(dbManager);
                    crow::response response = handler.listCompensationApprovals(req, userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(
                        dbManager, req, "boss", "薪酬审批列表", e.what(),
                        userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(
                    dbManager, req, res, "boss", "薪酬审批列表",
                    Permissions::kCompensationApprove,
                    userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/bosses/compensation-approvals/<int>/decision")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int proposalId)
            {
                int userId = -1;
                try
                {
                    userId = isValidPermissionToken(
                        req, res, dbManager, Permissions::kCompensationApprove);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(
                            dbManager, req, res, "boss", "薪酬审批决策");
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto jsonOpt = parser.parseJson(req, res);
                    if (jsonOpt)
                    {
                        bossHandler handler(dbManager);
                        crow::response response = handler.decideCompensationApproval(
                            req, userId, static_cast<long long>(proposalId), *jsonOpt);
                        ProcessHandlerResponse(req, res, response);
                    }
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(
                        dbManager, req, "boss", "薪酬审批决策", e.what(),
                        userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(
                    dbManager, req, res, "boss", "薪酬审批决策",
                    Permissions::kCompensationApprove,
                    userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });
}
