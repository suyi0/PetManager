#include "personnelRoutes.h"
#include "../../controllers/modules/personnel/personnelHandler.h"
#include "../../controllers/OperationLogger/OperationLogger.h"

void personnelRoutes::setupPersonnelRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }

    CROW_ROUTE(app, "/api/personnel/createDoctor")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPersonnelToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "创建医生");
                        return;
                    }

                    personnelHandler handler(dbManager);
                    crow::response response = handler.createDoctor(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "创建医生", "route exception", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "人事", "创建医生", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/personnel/deleteDoctor")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPersonnelToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "删除医生");
                        return;
                    }

                    personnelHandler handler(dbManager);
                    crow::response response = handler.deleteDoctor(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "删除医生", "route exception", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "人事", "删除医生", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/personnel/createWarehouserManager")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPersonnelToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "创建仓库管理员");
                        return;
                    }

                    personnelHandler handler(dbManager);
                    crow::response response = handler.createWarehouserManager(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "创建仓库管理员", "route exception", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "人事", "创建仓库管理员", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/personnel/deleteWarehouserManager")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidPersonnelToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "删除仓库管理员");
                        return;
                    }

                    personnelHandler handler(dbManager);
                    crow::response response = handler.deleteWarehouserManager(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "删除仓库管理员", "route exception", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "人事", "删除仓库管理员", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    routes_setup = true;
}
