#include "OrderHandler.h"

crow::response OrderHandler::getOrder(const crow::request& req)
{
    try{
        // 检查数据库连接是否存在
        if (!dbManager || !dbManager->getSchema() || !dbManager->getSession()) {
            return crow::response(500, R"({"error": "Database connection not available"})");
        }

        try{
            // 获取所有订单
            mysqlx::Table ordersTable = dbManager->getSchema()->getTable("orders");
            
            mysqlx::RowResult result = ordersTable.select().execute();

            nlohmann::json response_data = nlohmann::json::array();
            for (const auto& row : result) {
                nlohmann::json order_data;
                order_data.push_back({
                    {"order_id", row[0]},
                    {"patient_id", row[1]},
                    {"doctor_id", row[2]},
                    {"order_time", row[3]},
                    {"order_status", row[4]}
                });
                response_data.push_back(order_data);
            }

            return crow::response(200, response_data.dump());
        } catch (const mysqlx::Error &e) {
            return crow::response(500, R"({"error": "Failed to retrieve orders"})");
        }
    } catch (const std::exception &e) {
        return crow::response(500, R"({"error": "Internal server error"})");
    }
}