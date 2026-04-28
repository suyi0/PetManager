#include "financeRoutes.h"
#include "../../controllers/OperationLogger/OperationLogger.h"

void financeRoutes::setupFinanceRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }

    CROW_ROUTE(app, "/api/finance/homePageGetData")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if( res.code != 200 && userId == -1)
                    {
                        OperationLogger::LogAuthorizationFailure(dbManager, req, res, "财务", "获取首页数据");
                        return;
                    }

                    financeHandler handler(dbManager);
                    crow::response response = handler.homePageGetData(req);
                } catch (const std::exception& e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "财务", "获取首页数据", "route exception", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }

            });
}
