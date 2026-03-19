#include "OrderHandler.h"

crow::response OrderHandler::createOrder(const crow::request &req)
{
    crow::response res;
    auto request_body_opt = validateRequest(req, res);
    if (!request_body_opt)
        return res;
    auto &request_body = request_body_opt.value();

    try
    {
        // 检查必要字段
        if (!request_body.contains("pet_id") || !request_body.contains("doctor_id"))
        {
            return ResponseHelper::error(req, "Missing required fields");
        }

        int pet_id = request_body["pet_id"].get<int>();
        int doctor_id = request_body["doctor_id"].get<int>();

        std::string order_type = request_body.contains("order_type") ? request_body["order_type"].get<std::string>() : "";
        std::string order_date = request_body.contains("order_date") ? request_body["order_date"].get<std::string>() : "";

        int medicine_id = 0;
        int quantity = 0;
        std::string medicine_name = "";
        double medicine_price = 0.00;
        int stock_quantity = 0;

        if (request_body.contains("medicine_id"))
        {
            medicine_id = request_body["medicine_id"].get<int>();
        }
        if (request_body.contains("quantity"))
        {
            quantity = request_body["quantity"].get<int>();
        }

        if (pet_id <= 0 || doctor_id <= 0)
        {
            return ResponseHelper::error(req, "Invalid pet_id or doctor_id");
        }

        if (medicine_id <= 0 || quantity <= 0)
        {
            return ResponseHelper::error(req, "Invalid medicine_id or quantity");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            mysqlx::SqlResult warehouse = session->sql(
                                                     "SELECT item_name, item_price, stock "
                                                     "FROM warehouse "
                                                     "WHERE id = ? FOR UPDATE")
                                              .bind(medicine_id)
                                              .execute();

            if (warehouse.hasData())
            {
                mysqlx::Row row = warehouse.fetchOne();
                medicine_name = row[0].get<std::string>();
                medicine_price = row[1].get<double>();
                stock_quantity = row[2].get<int>();
            }

            if(stock_quantity < quantity)
            {
                return ResponseHelper::error(req, "库存不足!!!");
            }

            double order_totalprice = medicine_price * quantity;
            std::string order_status = "待付款";

            mysqlx::SqlResult orders = session->sql(
                                                  "INSERT INTO orders (pet_id, doctor_id, order_type, order_date, order_status, order_totalprice) VALUES "
                                                  "(?, ?, ?, ?, ?, ?)")
                                           .bind(pet_id, doctor_id, order_type, order_date, order_status, order_totalprice)
                                           .execute();

            unsigned long long orderId = orders.getAutoIncrementValue();

            mysqlx::SqlResult orderMedicines = session->sql(
                                                          "INSERT INTO orderMedicines (order_id, medicine_id, quantity, price) VALUES "
                                                          "(?, ?, ?, ?)")
                                                   .bind(orderId, medicine_id, quantity, medicine_price)
                                                   .execute();

            mysqlx::SqlResult updateStock = session->sql(
                                                       "UPDATE warehouse SET stock = stock - ? WHERE id = ?")
                                                .bind(quantity, medicine_id)
                                                .execute();
                                            
            session->sql("COMMIT").execute();

            nlohmann::json responseData;
            responseData["order_id"] = orderId;
            responseData["total_price"] = order_totalprice;
            return ResponseHelper::success(req, responseData);
        }
        catch (const std::exception &e)
        {
            session->sql("ROLLBACK").execute();
            return ResponseHelper::error(req, e.what());
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::error(req, e.what());
    }
}

crow::response OrderHandler::getOrderList(const crow::request &req, int &userId)
{
    try
    {
        crow::response res;
        // auto request_body_opt = validateRequest(req, res);
        // if (!request_body_opt)
        //     return res;
        // auto &request_body = request_body_opt.value();

        mysqlx::SqlResult result = dbManager->getSession()->sql(
                                                              "SELECT o.id, o.pet_id, p.pet_name, o.doctor_id, o.order_type, "
                                                              "o.order_date, o.order_status, o.order_totalprice, o.created_at "
                                                              "FROM orders as o "
                                                              "JOIN pets as p ON o.pet_id = p.id "
                                                              "WHERE p.user_id = ?")
                                       .bind(userId)
                                       .execute();

        nlohmann::json response_data = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json orderList;
            orderList["id"] = row[0].get<int>();
            orderList["pet_id"] = row[1].get<int>();
            orderList["pet_name"] = row[2].get<std::string>();
            orderList["doctor_id"] = row[3].get<int>();
            orderList["order_type"] = row[4].get<std::string>();
            orderList["order_date"] = row[5].get<std::string>();
            orderList["order_status"] = row[6].get<std::string>();
            orderList["order_totalprice"] = row[7].get<double>();
            orderList["created_at"] = row[8].get<std::string>();
            response_data.push_back(orderList);
        }
        return ResponseHelper::success(req, response_data);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response OrderHandler::getAllRecord(const crow::request &req, int &userId, int batch_size, int offset)
{
    try
    {
        crow::response res;
        // auto request_body_opt = validateRequest(req, res);
        // if (!request_body_opt)
        //     return res;
        // auto &request_body = request_body_opt.value();

        // 使用 JOIN 一次性查询，避免 N+1 查询问题
        mysqlx::SqlResult orders_result = dbManager->getSession()->sql(
                                                                     "SELECT o.id, o.pet_id, p.pet_name, o.doctor_id, o.order_type, "
                                                                     "o.order_date, o.order_status, o.order_totalprice, o.created_at "
                                                                     "FROM orders as o "
                                                                     "JOIN pets as p ON o.pet_id = p.id "
                                                                     "WHERE p.user_id = ? "
                                                                     "ORDER BY o.order_date DESC, o.created_at DESC "
                                                                     "LIMIT ? offset ? ")
                                              .bind(userId, batch_size, (offset - 1) * batch_size)
                                              .execute();

        nlohmann::json response_data = nlohmann::json::array();
        for (auto order_row : orders_result)
        {
            nlohmann::json order;
            order["id"] = order_row[0].get<int>();
            order["pet_id"] = order_row[1].get<int>();
            order["pet_name"] = order_row[2].get<std::string>();
            order["doctor_id"] = order_row[3].get<int>();
            order["order_type"] = order_row[4].get<std::string>();
            order["order_date"] = order_row[5].get<std::string>();
            order["order_status"] = order_row[6].get<std::string>();
            order["order_totalprice"] = order_row[7].get<double>();
            order["created_at"] = order_row[8].get<std::string>();

            // 第二步：查询该订单关联的药品信息
            int order_id = order["id"];
            mysqlx::SqlResult medicines_result = dbManager->getSession()->sql(
                                                                            "SELECT om.id, om.medicine_id, om.quantity, om.price, om.total_price, "
                                                                            "w.item_name, w.item_type "
                                                                            "FROM orderMedicines as om "
                                                                            "JOIN warehouse as w ON om.medicine_id = w.id "
                                                                            "WHERE om.order_id = ?")
                                                     .bind(order_id)
                                                     .execute();

            nlohmann::json medicines = nlohmann::json::array();
            for (auto med_row : medicines_result)
            {
                nlohmann::json medicine;
                medicine["id"] = med_row[0].get<int>();          // orderMedicines 表的 ID
                medicine["medicine_id"] = med_row[1].get<int>(); // warehouse 表的 ID
                medicine["quantity"] = med_row[2].get<int>();
                medicine["price"] = med_row[3].get<double>();
                medicine["total_price"] = med_row[4].get<double>();
                medicine["item_name"] = med_row[5].get<std::string>();
                medicine["item_type"] = med_row[6].get<std::string>();

                medicines.push_back(medicine);
            }

            // 将药品列表嵌套到订单对象中
            order["orderMedicines"] = medicines;

            response_data.push_back(order);
        }

        return ResponseHelper::success(req, response_data);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

nlohmann::json OrderHandler::getOrderData(const int &orderId)
{
    try
    {
        if (!dbManager || !dbManager->getSession() || !dbManager->getSchema())
        {
            return nlohmann::json(); // 返回空JSON表示错误
        }

        mysqlx::SqlResult result = dbManager->getSession()->sql("SELECT o.id, o.pet_id, p.pet_name, p.pet_type, p.pet_age, p.pet_sex, o.doctor_id, "
                                                                "o.order_type, o.order_date, o.order_status, o.order_totalprice, o.created_at"
                                                                "FROM orders o "
                                                                "JOIN pets as p ON o.pet_id = p.id"
                                                                "WHERE o.id = ?")
                                       .bind(orderId)
                                       .execute();

        if (result.count() == 0)
        {
            return nlohmann::json(); // 返回空JSON表示未找到
        }

        auto row = result.fetchOne();
        return {
            {"id", row[0].get<int>()},
            {"pet_id", row[1].get<int>()},
            {"doctor_id", row[2].get<int>()},
            {"pet_name", row[3].get<std::string>()},
            {"order_type", row[4].get<std::string>()},
            {"order_date", row[5].get<std::string>()},
            {"order_status", row[6].get<std::string>()},
            {"order_totalprice", row[7].get<double>()},
            {"created_at", row[8].get<std::string>()}};
    }
    catch (const std::exception &)
    {
        return nlohmann::json(); // 异常时返回空JSON
    }
}

crow::response OrderHandler::getOrderInformation(const crow::request &req, int &orderId)
{
    try
    {
        crow::response res;
        // auto request_body_opt = validateRequest(req, res);
        // if (!request_body_opt)
        //     return res;
        // auto &request_body = request_body_opt.value();

        // 反馈json
        nlohmann::json response = getOrderData(orderId);
        if (response.empty())
        {
            return ResponseHelper::notFound(req);
        }

        response["success"] = true;
        response["order"] = response;

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, "Internal server error" + std::string(e.what()));
    }
}

crow::response OrderHandler::changeOrder(const crow::request &req, int &orderId)
{
    crow::response res;
    auto request_body_opt = validateRequest(req, res);
    if (!request_body_opt)
        return res;
    auto &request_body = request_body_opt.value();

    try
    {
        int order_id = orderId; // 这个ID是从URL中获取的(更安全，不会被恶意修改)
        int DBpet_id = 0;
        int DBdoctor_id = 0;
        std::string DBorder_type = "";
        std::string DBorder_date = "";
        std::string DBorder_status = "";

        mysqlx::Table orders_table = dbManager->getSchema()->getTable("orders");
        mysqlx::RowResult orders_result = orders_table.select("id", "pet_id", "doctor_id", "order_type", "order_date", "order_status")
                                              .where("id = :order_id")
                                              .bind("order_id", order_id)
                                              .execute();
        if (orders_result.count() == 0)
        {
            return ResponseHelper::notFound(req, "订单不存在");
        }

        for (const auto &row : orders_result)
        {
            if (!row[1].isNull())
            {
                DBpet_id = row[1].get<int>();
            }
            if (!row[2].isNull())
            {
                DBdoctor_id = row[2].get<int>();
            }
            if (!row[3].isNull())
            {
                DBorder_type = clean_string(row[3].get<std::string>());
            }
            if (!row[4].isNull())
            {
                DBorder_date = clean_string(row[4].get<std::string>());
            }
            if (!row[5].isNull())
            {
                DBorder_status = clean_string(row[5].get<std::string>());
            }
        }

        // 创建数据库更新操作
        mysqlx::TableUpdate order_update = orders_table.update();
        bool has_changes = false;

        if (DBpet_id != request_body["pet_id"].get<int>())
        {
            order_update.set("pet_id", request_body["pet_id"].get<int>());
            has_changes = true;
        }
        if (DBdoctor_id != request_body["doctor_id"].get<int>())
        {
            order_update.set("doctor_id", request_body["doctor_id"].get<int>());
            has_changes = true;
        }
        if (DBorder_type != request_body["order_type"].get<std::string>())
        {
            order_update.set("order_type", request_body["order_type"].get<std::string>());
            has_changes = true;
        }
        if (DBorder_date != request_body["order_date"].get<std::string>())
        {
            order_update.set("order_date", request_body["order_date"].get<std::string>());
            has_changes = true;
        }
        if (DBorder_status != request_body["order_status"].get<std::string>())
        {
            order_update.set("order_status", request_body["order_status"].get<std::string>());
            has_changes = true;
        }

        if (has_changes)
        {
            order_update.where("order_id = :order_id")
                .bind("order_id", order_id)
                .execute();
            std::cout << "Order updated successfully" << std::endl;

            nlohmann::json response;
            response["success"] = true;
            response["message"] = "Form data saved successfully";
            response["data"] = getOrderData(order_id);

            return ResponseHelper::success(req, response);
        }
        else
        {
            std::cout << "No changes to update" << std::endl;
            return ResponseHelper::success(req, "No changes to update");
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error updating order: " << e.what() << std::endl;
        return ResponseHelper::error(req, e.what());
    }
}