#include "orderCommonHandler.h"
#include "RoleTypeUtils/RoleTypeUtils.h"

crow::response orderCommonHandler::getOrderSummary(const crow::request &req, int &userId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        const std::string roleName = RoleTypeUtils::getUserRoleName(dbManager, userId);
        const bool isBoss = RoleTypeUtils::isBossRole(roleName);
        const bool isMedicalStaff = RoleTypeUtils::isMedicalStaffRole(roleName);
        const std::string filterSql = isBoss
                                          ? ""
                                      : isMedicalStaff ? "WHERE o.doctor_id = ? "
                                                       : "WHERE o.owner_id = ? ";

        const std::string sql = "SELECT o.id, p.pet_name, COALESCE(d.name, ''), o.order_type, "
                                "o.order_data, o.order_status, o.order_totalprice, o.created_at "
                                "FROM orders as o "
                                "JOIN pets as p ON o.pet_id = p.id "
                                "LEFT JOIN users as d ON o.doctor_id = d.id " +
                                filterSql;

        auto query = dbManager->getSession()->sql(sql);
        if (!isBoss)
        {
            query.bind(userId);
        }

        mysqlx::SqlResult result = query.execute();

        nlohmann::json data = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json orderSummary;
            orderSummary["id"] = row[0].get<int>();
            orderSummary["pet_name"] = row[1].get<std::string>();
            orderSummary["doctor_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
            orderSummary["order_type"] = row[3].get<std::string>();
            orderSummary["order_data"] = row[4].get<std::string>();
            orderSummary["order_status"] = row[5].get<std::string>();
            orderSummary["order_totalprice"] = row[6].get<double>();
            orderSummary["created_at"] = row[7].get<std::string>();
            data.push_back(orderSummary);
        }
        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "订单", "获取订单列表", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response orderCommonHandler::getOrderRecords(const crow::request &req, int staffId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        const bool isBoss = RoleTypeUtils::userHasBossRole(dbManager, staffId);
        const std::string sql =
            "SELECT o.id, o.owner_id, COALESCE(owner.name, ''), "
            "o.pet_id, COALESCE(p.pet_name, ''), o.doctor_id, COALESCE(doctor.name, ''), "
            "COALESCE(o.order_type, ''), COALESCE(o.order_data, ''), "
            "COALESCE(o.order_status, '待付款'), COALESCE(o.order_totalprice, 0.0), "
            "CAST(o.created_at AS CHAR), CAST(o.updated_at AS CHAR), "
            "(SELECT COUNT(*) FROM orderMedicines AS om WHERE om.order_id = o.id) "
            "FROM orders AS o "
            "LEFT JOIN users AS owner ON owner.id = o.owner_id "
            "LEFT JOIN pets AS p ON p.id = o.pet_id "
            "LEFT JOIN users AS doctor ON doctor.id = o.doctor_id " +
            std::string(isBoss ? "" : "WHERE o.doctor_id = ? ") +
            "ORDER BY o.created_at DESC";

        auto query = dbManager->getSession()->sql(sql);
        if (!isBoss)
        {
            query.bind(staffId);
        }

        mysqlx::SqlResult result = query.execute();
        nlohmann::json response = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json item;
            item["id"] = row[0].get<int>();
            item["owner_id"] = row[1].get<int>();
            item["owner_name"] = row[2].get<std::string>();
            item["pet_id"] = row[3].get<int>();
            item["pet_name"] = row[4].get<std::string>();
            item["doctor_id"] = row[5].get<int>();
            item["doctor_name"] = row[6].get<std::string>();
            item["order_type"] = row[7].get<std::string>();
            item["order_data"] = row[8].get<std::string>();
            item["order_status"] = row[9].get<std::string>();
            item["order_totalprice"] = row[10].get<double>();
            item["created_at"] = row[11].get<std::string>();
            item["updated_at"] = row[12].get<std::string>();
            item["medicine_count"] = row[13].get<int>();
            response.push_back(item);
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "订单", "获取订单记录", e.what(), staffId > 0 ? std::optional<int>(staffId) : std::nullopt);
        return ResponseHelper::system_error(req, e.what());
    }
}

nlohmann::json orderCommonHandler::getOrderData(const int &orderId)
{
    try
    {
        mysqlx::SqlResult result = dbManager->getSession()->sql("SELECT o.id, o.owner_id, COALESCE(owner.name, ''), "
                                                                "o.pet_id, COALESCE(p.pet_name, ''), COALESCE(p.pet_type, ''), COALESCE(p.pet_age, ''), COALESCE(p.pet_sex, ''), "
                                                                "o.doctor_id, COALESCE(d.name, ''), COALESCE(o.order_type, ''), COALESCE(o.order_data, ''), "
                                                                "COALESCE(o.order_status, '待付款'), COALESCE(o.order_totalprice, 0.0), "
                                                                "CAST(o.created_at AS CHAR), CAST(o.updated_at AS CHAR) "
                                                                "FROM orders AS o "
                                                                "JOIN pets AS p ON o.pet_id = p.id "
                                                                "LEFT JOIN users AS owner ON o.owner_id = owner.id "
                                                                "LEFT JOIN users AS d ON o.doctor_id = d.id "
                                                                "WHERE o.id = ?")
                                       .bind(orderId)
                                       .execute();

        if (result.count() == 0)
        {
            return nlohmann::json(); // 返回空JSON表示未找到
        }

        auto row = result.fetchOne();
        nlohmann::json order = {
            {"id", row[0].get<int>()},
            {"owner_id", row[1].get<int>()},
            {"owner_name", row[2].get<std::string>()},
            {"pet_id", row[3].get<int>()},
            {"pet_name", row[4].get<std::string>()},
            {"pet_type", row[5].get<std::string>()},
            {"pet_age", row[6].get<std::string>()},
            {"pet_sex", row[7].get<std::string>()},
            {"doctor_id", row[8].get<int>()},
            {"doctor_name", row[9].get<std::string>()},
            {"order_type", row[10].get<std::string>()},
            {"order_data", row[11].get<std::string>()},
            {"order_status", row[12].get<std::string>()},
            {"order_totalprice", row[13].get<double>()},
            {"created_at", row[14].get<std::string>()},
            {"updated_at", row[15].get<std::string>()}};

        mysqlx::SqlResult medicinesResult = dbManager->getSession()
                                                ->sql("SELECT om.id, om.order_id, om.medicine_id, "
                                                      "COALESCE(NULLIF(om.medicine_name, ''), w.item_name, ''), COALESCE(w.item_type, ''), "
                                                      "om.quantity, COALESCE(om.price, 0.0), COALESCE(om.total_price, 0.0), "
                                                      "CAST(om.created_at AS CHAR), CAST(om.updated_at AS CHAR) "
                                                      "FROM orderMedicines AS om "
                                                      "LEFT JOIN warehouse AS w ON om.medicine_id = w.id "
                                                      "WHERE om.order_id = ? "
                                                      "ORDER BY id ASC")
                                                .bind(orderId)
                                                .execute();

        nlohmann::json medicines = nlohmann::json::array();
        for (auto medicineRow : medicinesResult)
        {
            nlohmann::json medicine;
            medicine["id"] = medicineRow[0].get<int>();
            medicine["order_id"] = medicineRow[1].get<int>();
            medicine["medicine_id"] = medicineRow[2].get<int>();
            medicine["medicine_name"] = medicineRow[3].get<std::string>();
            medicine["medicine_type"] = medicineRow[4].get<std::string>();
            medicine["quantity"] = medicineRow[5].get<int>();
            medicine["price"] = medicineRow[6].get<double>();
            medicine["total_price"] = medicineRow[7].get<double>();
            medicine["created_at"] = medicineRow[8].get<std::string>();
            medicine["updated_at"] = medicineRow[9].get<std::string>();
            medicines.push_back(medicine);
        }

        order["orderMedicines"] = medicines;
        return order;
    }
    catch (const std::exception &)
    {
        return nlohmann::json(); // 异常时返回空JSON
    }
}

crow::response orderCommonHandler::getOrderInformation(const crow::request &req, int &orderId)
{
    try
    {
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "订单", "获取订单详情", "database connection failed", orderId > 0 ? std::optional<int>(orderId) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        // 反馈json
        nlohmann::json response = getOrderData(orderId);
        if (response.empty())
        {
            return ResponseHelper::notFound(req);
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "订单", "获取订单详情", e.what(), orderId > 0 ? std::optional<int>(orderId) : std::nullopt);
        return ResponseHelper::system_error(req, "Internal server error" + std::string(e.what()));
    }
}
