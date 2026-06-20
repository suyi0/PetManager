#include "doctorHandler.h"
#include "../../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../../services/realtime/financeBroadcaster/financeHomeDataBroadcaster.h"
#include "statusLabelUtils/StatusLabelUtils.h"

#include <unordered_map>
#include <vector>

namespace
{
    std::string trimString(const std::string &value)
    {
        const auto start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
        {
            return "";
        }

        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }

}

crow::response doctorHandler::getDoctor(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        mysqlx::SqlResult result = dbManager->getSession()->sql("SELECT u.name, u.user_specialty, u.user_introduction, u.user_level "
                                                                "FROM users as u "
                                                                "JOIN onlineDoctors as od ON u.id = od.doctor_id "
                                                                "JOIN types as t ON u.type_id = t.id "
                                                                "WHERE t.type = '医生' AND od.status = 'online' AND u.is_deleted = 0")
                                       .execute();

        nlohmann::json response = nlohmann::json::array();

        for (auto row : result)
        {
            nlohmann::json doctor;
            doctor["name"] = row[0].get<std::string>();
            doctor["specialty"] = row[1].isNull() ? "" : row[1].get<std::string>();
            doctor["introduction"] = row[2].isNull() ? "" : row[2].get<std::string>();
            doctor["level"] = row[3].isNull() ? 0 : row[3].get<int>();
            response.push_back(doctor);
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response doctorHandler::getDutyStatus(const crow::request &req, int userId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        boost::posix_time::ptime currentDateTime = boost::posix_time::second_clock::local_time();
        std::string today = formatDateOnly(currentDateTime);

        mysqlx::SqlResult result = dbManager->getSession()->sql("SELECT date, check_in_time, check_out_time, status "
                                                                "FROM onlineDoctors "
                                                                "WHERE doctor_id = ? AND date = ? "
                                                                "LIMIT 1")
                                       .bind(userId, today)
                                       .execute();

        nlohmann::json response = nlohmann::json::object();
        response["is_online"] = false;
        response["date"] = today;
        response["check_in_time"] = "";
        response["check_out_time"] = "";
        response["status"] = "offline";

        auto row = result.fetchOne();
        if (row)
        {
            response["date"] = row[0].isNull() ? today : row[0].get<std::string>();
            response["check_in_time"] = row[1].isNull() ? "" : row[1].get<std::string>();
            response["check_out_time"] = row[2].isNull() ? "" : row[2].get<std::string>();
            response["status"] = row[3].isNull() ? "offline" : row[3].get<std::string>();
            response["is_online"] = response["status"] == "online";
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response doctorHandler::updateReservationStatus(const crow::request &req, int doctorId, int reservationId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
        {
            return res;
        }
        auto &request_body = request_body_opt.value();

        if (request_body.find("status") == request_body.end() || !request_body["status"].is_string())
        {
            return ResponseHelper::validation(req, "缺少预约状态");
        }

        const std::string status = request_body["status"].get<std::string>();
        if (!StatusLabelUtils::isValidReservationStatus(status))
        {
            return ResponseHelper::validation(req, "预约状态不合法");
        }
        const std::string dbStatus = StatusLabelUtils::toDbReservationStatus(status);

        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        const bool isBoss = RoleTypeUtils::userHasBossRole(dbManager, doctorId);
        mysqlx::SqlResult reservationResult = dbManager->getSession()
                                                  ->sql("SELECT doctor_id FROM reservations WHERE id = ? LIMIT 1")
                                                  .bind(reservationId)
                                                  .execute();
        auto reservationRow = reservationResult.fetchOne();
        if (!reservationRow)
        {
            return ResponseHelper::notFound(req, "未找到指定的预约记录");
        }

        const int recordDoctorId = reservationRow[0].isNull() ? 0 : reservationRow[0].get<int>();
        if (!isBoss && recordDoctorId != doctorId)
        {
            return ResponseHelper::permission_denied(req, "无权更新其他医生的预约记录");
        }

        mysqlx::SqlResult updateResult = dbManager->getSession()
                                             ->sql("UPDATE reservations SET status = ? WHERE id = ?")
                                             .bind(dbStatus)
                                             .bind(reservationId)
                                             .execute();

        if (updateResult.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req, "未找到指定的预约记录");
        }

        nlohmann::json response;
        response["message"] = "预约状态更新成功";
        response["reservation_id"] = reservationId;
        response["status"] = StatusLabelUtils::toDisplayReservationStatus(dbStatus);
        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response doctorHandler::createOrderRecord(const crow::request &req, int doctorId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int ownerId = request_body.value("ownerId", 0);
        int petId = request_body.value("petId", 0);
        std::string orderType = request_body.value("orderType", "");
        std::string orderData = request_body.value("orderData", "");
        double orderTotalPrice = request_body.value("orderTotalPrice", 0.0);

        nlohmann::json medicines = nlohmann::json::array();
        if (request_body.contains("orderMedicines"))
        {
            medicines = request_body["orderMedicines"];
        }

        if (!medicines.is_array())
        {
            return ResponseHelper::validation(req, "药品数据必须是数组");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        // 写入订单记录
        mysqlx::SqlResult ordersResult = session->sql("INSERT INTO orders ( "
                                                      "owner_id, pet_id, doctor_id, order_type, order_data, order_status, order_totalprice) "
                                                      "VALUES (?, ?, ?, ?, ?, ?, ?)")
                                             .bind(ownerId, petId, doctorId, orderType, orderData, "pending_payment", orderTotalPrice)
                                             .execute();

        unsigned long long orderId = ordersResult.getAutoIncrementValue();

        mysqlx::SqlResult medicinesResult;

        // 写入订单药品记录
        for (auto row : medicines)
        {
            medicinesResult = session->sql("INSERT INTO orderMedicines ( "
                                           "order_id, medicine_id, medicine_name, quantity, price, total_price) "
                                           "VALUES (?, ?, ?, ?, ?, ?)")
                                  .bind(orderId, row["medicineId"].get<int>(), row["medicineName"].get<std::string>(), row["quantity"].get<int>(), row["price"].get<double>(), row["totalPrice"].get<double>())
                                  .execute();
        }

        // 获取刚刚创建的订单记录
        mysqlx::SqlResult createdOrderResult = session->sql("SELECT o.id, COALESCE(p.pet_name, ''), COALESCE(owner.name, ''), COALESCE(doctor.name, ''), "
                                                            "CAST(o.created_at AS CHAR), COALESCE(o.order_totalprice, 0.0), "
                                                            "COALESCE(o.order_status, 'pending_payment') "
                                                            "FROM orders AS o "
                                                            "LEFT JOIN pets AS p ON o.pet_id = p.id "
                                                            "LEFT JOIN users AS owner ON owner.id = o.owner_id "
                                                            "LEFT JOIN users AS doctor ON doctor.id = o.doctor_id "
                                                            "WHERE o.id = ? "
                                                            "LIMIT 1")
                                                   .bind(orderId)
                                                   .execute();

        auto createdOrderRow = createdOrderResult.fetchOne();
        if (!createdOrderRow)
        {
            session->sql("ROLLBACK").execute();
            return ResponseHelper::system_error(req, "订单创建后查询失败");
        }

        session->sql("COMMIT").execute();

        nlohmann::json orderRecord;
        orderRecord["id"] = createdOrderRow[0].isNull() ? 0 : createdOrderRow[0].get<int>();
        orderRecord["petName"] = createdOrderRow[1].isNull() ? "" : createdOrderRow[1].get<std::string>();
        orderRecord["ownerName"] = createdOrderRow[2].isNull() ? "" : createdOrderRow[2].get<std::string>();
        orderRecord["doctorName"] = createdOrderRow[3].isNull() ? "" : createdOrderRow[3].get<std::string>();
        orderRecord["createdAt"] = createdOrderRow[4].isNull() ? "" : createdOrderRow[4].get<std::string>();
        orderRecord["totalFee"] = createdOrderRow[5].isNull() ? 0.0 : createdOrderRow[5].get<double>();
        orderRecord["status"] = StatusLabelUtils::toDisplayOrderStatus(createdOrderRow[6].isNull() ? "pending_payment" : createdOrderRow[6].get<std::string>());
        orderRecord["orderMedicines"] = medicines;

        FinanceHomeDataBroadcaster::instance().notifyHomeDataChanged();
        AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
        return ResponseHelper::success(req, orderRecord);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response doctorHandler::getUserList(const crow::request &req, const std::string data, const std::string &identifier)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        if (data.empty())
        {
            return ResponseHelper::validation(req, "搜索关键词不能为空");
        }

        mysqlx::SqlResult result;
        if (identifier == "name")
        {
            result = dbManager->getSession()->sql("SELECT u.id, u.type_id, u.name, p.phone, u.email, u.head_image "
                                                  "FROM users AS u "
                                                  "LEFT JOIN phones AS p ON p.user_id = u.id "
                                                  "WHERE u.name LIKE ? AND u.is_deleted = 0 "
                                                  "ORDER BY u.name ASC "
                                                  "LIMIT 20")
                         .bind(data + "%")
                         .execute();
        }
        else if (identifier == "phone")
        {
            const std::string phoneLike = "%" + data + "%";
            result = dbManager->getSession()
                         ->sql("SELECT DISTINCT u.id, u.type_id, u.name, p.phone, u.email, u.head_image "
                               "FROM users AS u "
                               "LEFT JOIN phones AS p ON u.id = p.user_id "
                               "WHERE u.is_deleted = 0 "
                               "AND (COALESCE(p.phone, '') LIKE ? "
                               "OR COALESCE(p.phone_lastfour, '') LIKE ?) "
                               "ORDER BY u.name ASC "
                               "LIMIT 20")
                         .bind(phoneLike, phoneLike)
                         .execute();

            if (result.count() == 0)
            {
                return ResponseHelper::error(req, "用户不存在");
            }
        }
        else
        {
            return ResponseHelper::validation(req, "identifier 仅支持 name 或 phone");
        }

        nlohmann::json response = nlohmann::json::array();
        std::unordered_map<int, std::size_t> userIndexById; // 无序哈希表，作用快速根据用户ID找到对应的用户在响应数组中的位置
        std::vector<int> userIds;                           // 存储用户ID列表，方便后续查询宠物信息
        for (auto row : result)
        {
            const int userId = row[0].get<int>();

            nlohmann::json user;
            user["id"] = userId;
            user["type_id"] = row[1].isNull() ? 0 : row[1].get<int>();
            user["name"] = row[2].isNull() ? "" : row[2].get<std::string>();
            user["phone"] = row[3].isNull() ? "" : row[3].get<std::string>();
            user["email"] = row[4].isNull() ? "" : row[4].get<std::string>();
            user["head_image"] = row[5].isNull() ? "" : row[5].get<std::string>();
            user["pets"] = nlohmann::json::array();

            response.push_back(user);
            userIndexById[userId] = response.size() - 1; // 记录用户ID与响应数组索引的映射关系
            userIds.push_back(userId);                   // 搜集用户ID
        }

        // 根据用户ID列表批量查询宠物信息，避免在循环内对每个用户单独查询造成的性能问题
        if (!userIds.empty())
        {
            std::string placeholders; // 用于构建SQL查询中的占位符字符串，例如 "?, ?, ?" 以匹配用户ID列表的长度，以便一次性查询20个用户的全部相关宠物信息
            for (std::size_t index = 0; index < userIds.size(); ++index)
            {
                placeholders += index == 0 ? "?" : ", ?";
            }

            auto petQuery = dbManager->getSession()
                                ->sql("SELECT id, user_id, COALESCE(pet_name, '') "
                                      "FROM pets "
                                      "WHERE user_id IN (" +
                                      placeholders + ") AND is_deleted = 0 "
                                                     "ORDER BY user_id ASC, created_at ASC, id ASC");

            for (const int userId : userIds)
            {
                petQuery.bind(userId);
            }

            mysqlx::SqlResult petResult = petQuery.execute();
            for (auto row : petResult)
            {
                const int userId = row[1].get<int>();
                auto userIndexIt = userIndexById.find(userId);
                // 快速通过宠物表的用户ID找到对应的用户在响应数组中的位置，避免再次遍历响应数组查找用户对象，提高性能

                if (userIndexIt == userIndexById.end())
                {
                    continue;
                }

                nlohmann::json pet;
                pet["id"] = row[0].get<int>();
                pet["pet_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
                // it->first response的键     it->second response的键值
                response[userIndexIt->second]["pets"].push_back(pet);
            }
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response doctorHandler::searchMedicines(const crow::request &req, const std::string &keyword)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        const std::string normalizedKeyword = trimString(keyword); // trimString() 函数用于去除字符串首尾的空白字符
        const std::string likeKeyword = "%" + normalizedKeyword + "%";

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT id, item_name, item_type, item_price, item_number, CAST(item_expirationdate AS CHAR) "
                                             "FROM warehouse "
                                             "WHERE is_deleted = 0 AND item_number > 0 "
                                             "AND (? = '' OR item_name LIKE ? OR item_type LIKE ?) "
                                             "ORDER BY item_name ASC, id DESC "
                                             "LIMIT 50")
                                       .bind(normalizedKeyword, likeKeyword, likeKeyword)
                                       .execute();

        nlohmann::json response = nlohmann::json::array();
        for (auto row : result)
        {
            const std::string expirationDate = row[5].isNull() ? "" : row[5].get<std::string>();

            nlohmann::json medicine;
            medicine["id"] = row[0].isNull() ? 0 : row[0].get<int>();
            medicine["name"] = row[1].isNull() ? "" : clean_string(row[1].get<std::string>());
            medicine["type"] = row[2].isNull() ? "" : row[2].get<std::string>();
            medicine["price"] = row[3].isNull() ? 0.0 : row[3].get<double>();
            medicine["stock"] = row[4].isNull() ? 0 : row[4].get<int>();
            medicine["spec"] = expirationDate.empty() ? "库存药品" : "有效期至 " + expirationDate;
            response.push_back(medicine);
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, "搜索药品失败: " + std::string(e.what()));
    }
}

// 医生端获取用户详细档案信息，包括基本信息、宠物信息和订单信息
crow::response doctorHandler::getUserProfiles(const crow::request &req, int userId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        mysqlx::SqlResult users = dbManager->getSession()
                                      ->sql("SELECT u.id, COALESCE(u.name, ''), COALESCE(p.phone, ''), COALESCE(u.email, ''), "
                                            "CAST(u.birthday AS CHAR), COALESCE(u.head_image, ''), COALESCE(u.user_specialty, ''), "
                                            "COALESCE(u.user_introduction, ''), COALESCE(u.user_level, 0), COALESCE(u.funds, 0.0), "
                                            "CAST(u.created_at AS CHAR), COALESCE(t.type, ''), COALESCE(u.type_id, 0) "
                                            "FROM users AS u "
                                            "LEFT JOIN types AS t ON u.type_id = t.id "
                                            "LEFT JOIN phones AS p ON p.user_id = u.id "
                                            "WHERE u.id = ? ")
                                      .bind(userId)
                                      .execute();

        nlohmann::json response = nlohmann::json::array();
        std::unordered_map<int, std::size_t> userIndexById;
        std::vector<int> userIds;
        for (auto row : users)
        {
            const int userId = row[0].get<int>();

            nlohmann::json user;
            user["id"] = userId;
            user["type_id"] = row[12].isNull() ? 0 : row[12].get<int>();
            user["type_name"] = row[11].isNull() ? "" : row[11].get<std::string>();
            user["name"] = row[1].isNull() ? "" : row[1].get<std::string>();
            user["phone"] = row[2].isNull() ? "" : row[2].get<std::string>();
            user["email"] = row[3].isNull() ? "" : row[3].get<std::string>();
            user["birthday"] = row[4].isNull() ? "" : row[4].get<std::string>();
            user["head_image"] = row[5].isNull() ? "" : row[5].get<std::string>();
            user["user_specialty"] = row[6].isNull() ? "" : row[6].get<std::string>();
            user["user_introduction"] = row[7].isNull() ? "" : row[7].get<std::string>();
            user["user_level"] = row[8].isNull() ? 0 : row[8].get<int>();
            user["funds"] = row[9].isNull() ? 0.0 : row[9].get<double>();
            user["created_at"] = row[10].isNull() ? "" : row[10].get<std::string>();
            user["pets"] = nlohmann::json::array();
            user["orders"] = nlohmann::json::array();
            response.push_back(user);
            userIndexById[userId] = response.size() - 1;
            userIds.push_back(userId);
        }

        if (!userIds.empty())
        {
            std::string placeholders; // 用于构建SQL查询中的占位符字符串，例如 "?, ?, ?" 以匹配用户ID列表的长度，以便一次性查询20个用户的全部相关宠物信息
            for (std::size_t index = 0; index < userIds.size(); ++index)
            {
                placeholders += index == 0 ? "?" : ", ?";
            }

            auto petQuery = dbManager->getSession()
                                ->sql("SELECT id, user_id, COALESCE(pet_name, ''), COALESCE(pet_type, ''), "
                                      "COALESCE(pet_sex, '') "
                                      "FROM pets "
                                      "WHERE user_id IN (" +
                                      placeholders + ") AND is_deleted = 0 "
                                                     "ORDER BY user_id ASC, created_at ASC, id ASC");

            auto orderQuery = dbManager->getSession()
                                  ->sql("SELECT o.id, o.owner_id, COALESCE(p.pet_name, ''), COALESCE(doctor.name, ''), "
                                        "COALESCE(o.order_type, ''), COALESCE(o.order_data, ''), "
                                        "COALESCE(o.order_status, 'pending_payment'), COALESCE(o.order_totalprice, 0.0) "
                                        "FROM orders AS o "
                                        "LEFT JOIN pets AS p ON o.pet_id = p.id "
                                        "LEFT JOIN users AS doctor ON doctor.id = o.doctor_id "
                                        "WHERE o.owner_id IN (" +
                                        placeholders + ") "
                                                       "ORDER BY o.created_at DESC, o.id DESC");

            for (const int userId : userIds)
            {
                petQuery.bind(userId);
                orderQuery.bind(userId);
            }

            mysqlx::SqlResult petResult = petQuery.execute();
            mysqlx::SqlResult orderResult = orderQuery.execute();
            for (auto row : petResult)
            {
                const int ownerId = row[1].get<int>();
                auto userIndexIt = userIndexById.find(ownerId);
                // 快速通过用户ID找到对应的用户在响应数组中的位置，避免再次遍历响应数组查找用户对象，提高性能

                if (userIndexIt == userIndexById.end())
                {
                    continue;
                }

                nlohmann::json pet;
                pet["id"] = row[0].get<int>();
                pet["pet_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
                pet["pet_type"] = row[3].isNull() ? "" : row[3].get<std::string>();
                pet["pet_sex"] = row[4].isNull() ? "" : row[4].get<std::string>();
                // it->first response的键     it->second response的键值
                response[userIndexIt->second]["pets"].push_back(pet);
            }

            for (auto row : orderResult)
            {
                const int ownerId = row[1].get<int>();
                auto userIndexIt = userIndexById.find(ownerId);

                if (userIndexIt == userIndexById.end())
                {
                    continue;
                }

                nlohmann::json order;
                order["id"] = row[0].get<int>();
                order["pet_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
                order["doctor_name"] = row[3].isNull() ? "" : row[3].get<std::string>();
                order["order_type"] = row[4].isNull() ? "" : row[4].get<std::string>();
                order["order_data"] = row[5].isNull() ? "" : row[5].get<std::string>();
                order["order_status"] = StatusLabelUtils::toDisplayOrderStatus(row[6].isNull() ? "pending_payment" : row[6].get<std::string>());
                order["order_totalprice"] = row[7].isNull() ? 0.0 : row[7].get<double>();

                response[userIndexIt->second]["orders"].push_back(order);
            }
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response doctorHandler::handleDutyAction(const crow::request &req, int userId, bool requireDoctorId)
{
    crow::response res;
    auto request_body_opt = validateRequest(req, res);
    if (!request_body_opt)
    {
        return res;
    }
    auto &request_body = request_body_opt.value();

    const std::string status = request_body.value("status", "");
    const int targetDoctorId = requireDoctorId
                                   ? request_body.value("doctorId", -1)
                                   : request_body.value("doctorId", userId);

    if (targetDoctorId <= 0)
    {
        return ResponseHelper::validation(req, "Missing or invalid doctorId");
    }

    if (status == "online")
    {
        return onlineDoctor(req, targetDoctorId);
    }

    if (status == "offline")
    {
        return offlineDoctor(req, targetDoctorId);
    }

    return ResponseHelper::error(req, "status 参数无效，仅支持 online 或 offline");
}

crow::response doctorHandler::onlineDoctor(const crow::request &req, int userId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        boost::posix_time::ptime onlineDateTime = boost::posix_time::second_clock::local_time();
        std::string date = formatDateOnly(onlineDateTime);
        std::string time = formatTimeOnly(onlineDateTime);
        std::string check_in_time_end;

        mysqlx::SqlResult workTime_result = dbManager->getSession()
                                                ->sql("SELECT check_in_time_end FROM workTimes")
                                                .execute();

        auto work_time_row = workTime_result.fetchOne();
        if (!work_time_row)
        {
            return ResponseHelper::error(req, "未配置签到时间，请先联系管理员设置");
        }

        check_in_time_end = work_time_row[1].get<std::string>();

        if (time >= check_in_time_end)
        {
            return ResponseHelper::error(req, "已超过签到时间，如果有特殊情况导致请与管理人员确认!!!");
        }
        else
        {
            mysqlx::SqlResult existing_result = dbManager->getSession()->sql("SELECT status "
                                                                             "FROM onlineDoctors "
                                                                             "WHERE doctor_id = ? "
                                                                             "LIMIT 1")
                                                    .bind(userId)
                                                    .execute();

            auto existing_row = existing_result.fetchOne();
            if (existing_row)
            {
                std::string current_status = existing_row[0].get<std::string>();

                if (current_status == "online")
                {
                    return ResponseHelper::error(req, "当前已处于签到状态，请勿重复签到!");
                }

                dbManager->getSession()->sql("UPDATE onlineDoctors "
                                             "SET date = ?, check_in_time = ?, check_out_time = NULL, status = 'online' "
                                             "WHERE doctor_id = ?")
                    .bind(date, time, userId)
                    .execute();
            }
            else
            {
                dbManager->getSession()->sql("INSERT INTO onlineDoctors (doctor_id, date, check_in_time, status) "
                                             "VALUES (?, ?, ?, 'online')")
                    .bind(userId, date, time)
                    .execute();
            }

            AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
            return ResponseHelper::success(req, "签到成功!");
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response doctorHandler::offlineDoctor(const crow::request &req, int userId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        boost::posix_time::ptime offlineDateTime = boost::posix_time::microsec_clock::local_time();
        std::string date = formatDateOnly(offlineDateTime);
        std::string time = formatTimeOnly(offlineDateTime);
        std::string check_in_time_start;
        std::string check_in_time_end;
        std::string check_out_time_start;
        std::string check_out_time_end;

        mysqlx::SqlResult workTime_result = dbManager->getSession()
                                                ->sql("SELECT check_in_time_start, check_in_time_end, check_out_time_start, check_out_time_end FROM workTimes")
                                                .execute();

        auto work_time_row = workTime_result.fetchOne();
        if (!work_time_row)
        {
            return ResponseHelper::error(req, "未配置签退时间，请先联系管理员设置");
        }

        check_in_time_start = work_time_row[0].get<std::string>();
        check_in_time_end = work_time_row[1].get<std::string>();
        check_out_time_start = work_time_row[2].get<std::string>();
        check_out_time_end = work_time_row[3].get<std::string>();

        if (time <= check_out_time_start)
        {
            return ResponseHelper::error(req, "未到签退时间，如要提前签退请与管理人员确认!!!");
        }
        else if (time >= check_out_time_end)
        {
            return ResponseHelper::error(req, "已超过签退时间，如果有特殊情况导致请与管理人员确认!!!");
        }
        else
        {
            auto result = dbManager->getSession()->sql("UPDATE onlineDoctors "
                                                       "SET check_out_time = ?, status = 'offline' "
                                                       "WHERE doctor_id = ? AND date = ? AND status = 'online'")
                              .bind(time, userId, date)
                              .execute();

            // 检查是否更新了记录
            if (result.getAffectedItemsCount() > 0)
            {
                mysqlx::SqlResult workTime_row = dbManager->getSession()
                                                     ->sql("SELECT doctor_id, date, check_in_time, check_out_time "
                                                           "FROM workTimeLogs "
                                                           "WHERE doctor_id = ? AND date = ?")
                                                     .bind(userId, date)
                                                     .execute();

                auto row = workTime_row.fetchOne();
                const int doctorId = row[0].isNull() ? 0 : row[0].get<int>();
                const std::string workTime_date = row[1].isNull() ? "" : row[1].get<std::string>();
                const std::string checkInTime = row[2].isNull() ? "" : row[2].get<std::string>();
                const std::string checkOutTime = row[3].isNull() ? "" : row[3].get<std::string>();
                std::string workTime_status = "normal";
                if (checkInTime.empty() || checkOutTime.empty())
                {
                    workTime_status = "abnormal";
                }
                else if (checkInTime >= check_in_time_end)
                {
                    workTime_status = "late";
                }
                else if (checkOutTime <= check_out_time_start)
                {
                    workTime_status = "early_leave";
                }
                else if (checkOutTime >= check_out_time_end)
                {
                    workTime_status = "overtime";
                }

                mysqlx::SqlResult workTimeLogs_result = dbManager->getSession()
                                                            ->sql("INSERT INTO workTimeLogs (doctor_id, date, check_in_time, check_out_time, status) "
                                                                  "VALUES (?, ?, ?, ?, ?)")
                                                            .bind(doctorId, workTime_date, checkInTime, checkOutTime, workTime_status)
                                                            .execute();

                AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
                return ResponseHelper::success(req, "签退成功!");
            }
            else
            {
                return ResponseHelper::error(req, "签退失败,请联系管理员!!!");
            }
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

nlohmann::json doctorHandler::getOrderData(const int &orderId)
{
    try
    {
        if (!checkDbConnection())
        {
            return nlohmann::json(); // 返回空JSON表示错误
        }

        mysqlx::SqlResult result = dbManager->getSession()->sql("SELECT o.id, o.pet_id, p.pet_name, p.pet_type, p.pet_age, p.pet_sex, o.doctor_id, "
                                                                "o.order_type, o.order_data, o.order_status, o.order_totalprice, o.created_at "
                                                                "FROM orders o "
                                                                "JOIN pets as p ON o.pet_id = p.id "
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
            {"pet_name", row[2].get<std::string>()},
            {"pet_type", row[3].get<std::string>()},
            {"pet_age", row[4].get<std::string>()},
            {"pet_sex", row[5].get<std::string>()},
            {"doctor_id", row[6].get<int>()},
            {"order_type", row[7].get<std::string>()},
            {"order_data", row[8].get<std::string>()},
            {"order_status", StatusLabelUtils::toDisplayOrderStatus(row[9].get<std::string>())},
            {"order_totalprice", row[10].get<double>()},
            {"created_at", row[11].get<std::string>()}};
    }
    catch (const std::exception &)
    {
        return nlohmann::json(); // 异常时返回空JSON
    }
}
crow::response doctorHandler::changeOrder(const crow::request &req, int &orderId)
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
        std::string DBorder_data = "";
        std::string DBorder_status = "";

        mysqlx::SqlResult orders_result = dbManager->getSession()
                                              ->sql("SELECT id, pet_id, doctor_id, order_type, order_data, order_status "
                                                    "FROM orders WHERE id = ?")
                                              .bind(order_id)
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
                DBorder_type = row[3].get<std::string>();
            }
            if (!row[4].isNull())
            {
                DBorder_data = row[4].get<std::string>();
            }
            if (!row[5].isNull())
            {
                DBorder_status = row[5].get<std::string>();
            }
        }

        bool has_changes = false;

        if (DBpet_id != request_body["pet_id"].get<int>())
        {
            has_changes = true;
        }
        if (DBdoctor_id != request_body["doctor_id"].get<int>())
        {
            has_changes = true;
        }
        if (DBorder_type != request_body["order_type"].get<std::string>())
        {
            has_changes = true;
        }
        if (DBorder_data != request_body["order_data"].get<std::string>())
        {
            has_changes = true;
        }
        const std::string requestOrderStatus = StatusLabelUtils::toDbOrderStatus(request_body["order_status"].get<std::string>());
        if (!StatusLabelUtils::isValidOrderStatus(requestOrderStatus))
        {
            return ResponseHelper::validation(req, "订单状态不合法");
        }

        if (DBorder_status != requestOrderStatus)
        {
            has_changes = true;
        }

        if (has_changes)
        {
            dbManager->getSession()
                ->sql("UPDATE orders SET pet_id = ?, doctor_id = ?, order_type = ?, order_data = ?, order_status = ? WHERE id = ?")
                .bind(
                    request_body["pet_id"].get<int>(),
                    request_body["doctor_id"].get<int>(),
                    request_body["order_type"].get<std::string>(),
                    request_body["order_data"].get<std::string>(),
                    requestOrderStatus,
                    order_id)
                .execute();
            std::cout << "Order updated successfully" << std::endl;
            FinanceHomeDataBroadcaster::instance().notifyHomeDataChanged();
            AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
            return ResponseHelper::success(req, getOrderData(order_id));
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
        OperationLogger::LogExceptionOperation(dbManager, req, "订单", "修改订单", e.what());
        return ResponseHelper::error(req, e.what());
    }
}

nlohmann::json doctorHandler::buildQueuesData(const int &doctorId)
{
    auto session = dbManager->getSession();
    nlohmann::json queues = nlohmann::json::array();

    mysqlx::SqlResult result = session->sql(
                                       "SELECT q.id, q.queue_date, q.queue_number, q.owner_id, q.pet_id, "
                                       "COALESCE(p.pet_name, ''), COALESCE(p.pet_sex, ''), "
                                       "COALESCE(p.pet_breed, ''), COALESCE(p.pet_age, ''), "
                                       "COALESCE(owner.name, ''), q.status, q.source, q.triage_level, "
                                       "COALESCE(DATE_FORMAT(COALESCE(q.arrived_at, q.scheduled_at, q.created_at), '%Y-%m-%d %H:%i'), '') "
                                       "FROM medicalQueues AS q "
                                       "LEFT JOIN pets AS p ON q.pet_id = p.id "
                                       "LEFT JOIN users AS owner ON q.owner_id = owner.id "
                                       "WHERE q.doctor_id = ? "
                                       "AND q.is_deleted = 0 "
                                       "AND q.queue_date = CURRENT_DATE "
                                       "AND q.status IN ('waiting', 'in_progress', 'skipped') "
                                       "ORDER BY FIELD(q.triage_level, 'urgent', 'priority', 'normal'), "
                                       "COALESCE(q.arrived_at, q.scheduled_at, q.created_at) ASC, q.id ASC")
                                   .bind(doctorId)
                                   .execute();

    for (const auto &row : result)
    {
        nlohmann::json queue;
        queue["id"] = row[0].get<int>();
        queue["queueDate"] = row[1].isNull() ? "" : row[1].get<std::string>();
        queue["queue_date"] = queue["queueDate"];
        queue["queueNumber"] = row[2].isNull() ? "" : row[2].get<std::string>();
        queue["queue_number"] = queue["queueNumber"];
        queue["ownerId"] = row[3].get<int>();
        queue["owner_id"] = queue["ownerId"];
        queue["petId"] = row[4].get<int>();
        queue["pet_id"] = queue["petId"];
        queue["petName"] = row[5].isNull() ? "" : row[5].get<std::string>();
        queue["pet_name"] = queue["petName"];
        queue["sex"] = row[6].isNull() ? "" : row[6].get<std::string>();
        queue["breed"] = row[7].isNull() ? "" : row[7].get<std::string>();
        queue["age"] = row[8].isNull() ? "" : row[8].get<std::string>();
        queue["ownerName"] = row[9].isNull() ? "" : row[9].get<std::string>();
        queue["owner_name"] = queue["ownerName"];
        queue["status"] = StatusLabelUtils::toDisplayMedicalQueueStatus(row[10].isNull() ? "waiting" : row[10].get<std::string>());
        queue["source"] = StatusLabelUtils::toDisplayMedicalQueueSource(row[11].isNull() ? "walk_in" : row[11].get<std::string>());
        queue["level"] = StatusLabelUtils::toDisplayTriageLevel(row[12].isNull() ? "normal" : row[12].get<std::string>());
        queue["triageLevel"] = queue["level"];
        queue["triage_level"] = queue["level"];
        queue["arrivedAt"] = row[13].isNull() ? "" : row[13].get<std::string>();
        queue["scheduledOrArrivedAt"] = queue["arrivedAt"];
        queue["symptom"] = "";
        queues.push_back(queue);
    }

    return queues;
}

crow::response doctorHandler::getQueues(const crow::request &req, const int doctorId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        return ResponseHelper::success(req, buildQueuesData(doctorId));
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}
