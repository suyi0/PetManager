#include "OrderRoutes.h"
#include "../../controllers/OperationLogger/OperationLogger.h"

void OrderRoutes::setupOrderRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
        return;

    // 创建订单
    CROW_ROUTE(app, "/api/order/createOrder")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            int userId = -1;
            try {
                userId = isValidUserToken(req, res, dbManager);
                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "订单", "创建订单");
                    return;
                }
                OrderHandler handler(dbManager);
                crow::response response = handler.createOrder(req);
                ProcessHandlerResponse(req, res, response);
            }
            catch(const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "订单", "创建订单", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "订单", "创建订单", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 获得订单列表
    CROW_ROUTE(app, "/api/order/getOrderList")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1) {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "订单", "获取订单列表");
                    return;
                }
                OrderHandler handler(dbManager);
                crow::response response = handler.getOrderList(req, userId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "订单", "获取订单列表", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "订单", "获取订单列表", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);    });

    CROW_ROUTE(app, "/api/order/getAllRecord")
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

                OrderHandler handler(dbManager);
                crow::response response = handler.getAllRecord(req, userId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "订单", "获取全部病历", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "订单", "获取全部病历", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 获得订单信息
    CROW_ROUTE(app, "/api/order/getOrderInformation/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int orderId)
                                                                   {
            int userId = -1;
            try {
                userId = isValidUserorderToken(req, res, orderId, dbManager);
                
                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "订单", "获取订单详情");
                    return;
                }

                OrderHandler handler(dbManager);
                crow::response response = handler.getOrderInformation(req, orderId);
                ProcessHandlerResponse(req, res, response);

            }
            catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "订单", "获取订单详情", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "订单", "获取订单详情", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 修改订单
    CROW_ROUTE(app, "/api/order/changeOrder/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int orderId)
                                                                   {
            int userId = -1;
            try
            {
                // 验证用户token权限
                userId = isValidUserorderToken(req, res, orderId, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "订单", "修改订单");
                    return;
                }
                
                // 验证成功进行订单处理
                OrderHandler handler(dbManager);
                crow::response response = handler.changeOrder(req, orderId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "订单", "修改订单", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));

            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "订单", "修改订单", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    routes_setup = true;
}
