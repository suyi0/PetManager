#include "OrderRoutes.h"

void OrderRoutes::setupOrderRoutes(CrowApp& app, DatabaseManagerInterface* dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
        return;

    // 创建 OrderHandler 实例
    OrderHandler handler(dbManager);

    // 获得订单记录
    CROW_ROUTE(app, "/api/order/getrecord")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            try {
                // 检查数据库连接是否存在
                if (!dbManager || !dbManager->getSchema() || !dbManager->getSession()) {
                    res.code = 500;
                    res.write(R"({"error": "Database connection not available"})");
                    res.end();
                    return;
                }
            } catch (const std::exception& e) {
                res.code = 500;
                res.write(R"({"error": "Failed to check database connection"})");
            } });

    routes_setup = true;
}