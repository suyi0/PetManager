#include "OrderRoutes.h"

void OrderRoutes::setupOrderRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
        return;

    // 创建订单
    CROW_ROUTE(app, "/api/order/createOrder")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try {
                OrderHandler handler(dbManager);
                crow::response response = handler.createOrder(req);
                ProcessHandlerResponse(req, res, response);
            }
            catch(const std::exception& e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end(); });

    // 获得订单列表
    CROW_ROUTE(app, "/api/order/getOrderList")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            try
            {
                int userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1) {
                    res.end();
                    return;
                }
                OrderHandler handler(dbManager);
                crow::response response = handler.getOrderList(req, userId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception& e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();    });

    CROW_ROUTE(app, "/api/order/getAllRecord")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            try
            {
                int userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    res.end();
                    return;
                }

                OrderHandler handler(dbManager);
                crow::response response = handler.getAllRecord(req, userId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception& e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end(); });

    // 获得订单信息
    CROW_ROUTE(app, "/api/order/getOrderInformation/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int orderId)
                                                                   {
            try {
                int userId = isValidUserorderToken(req, res, orderId, dbManager);
                
                if(res.code != 200 || userId == -1)
                {
                    res.end();
                    return;
                }

                OrderHandler handler(dbManager);
                crow::response response = handler.getOrderInformation(req, orderId);
                ProcessHandlerResponse(req, res, response);

            }
            catch (const std::exception& e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end(); });

    // 修改订单
    CROW_ROUTE(app, "/api/order/changeOrder/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int orderId)
                                                                   {
            try
            {
                // 验证用户token权限
                int userId = isValidUserorderToken(req, res, orderId, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    res.end();
                    return;
                }
                
                // 验证成功进行订单处理
                OrderHandler handler(dbManager);
                crow::response response = handler.changeOrder(req, orderId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));

            }
            res.end(); });

    routes_setup = true;
}
