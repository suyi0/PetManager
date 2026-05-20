#include "doctorHandler.h"
#include "RoleTypeUtils/RoleTypeUtils.h"

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
                                                                "WHERE t.type = '医生' AND od.status = 'online'")
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
                                       .bind(userId)
                                       .bind(today)
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

crow::response doctorHandler::getUserProfiles(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        mysqlx::SqlResult users = dbManager->getSession()
                                      ->sql("SELECT u.id, COALESCE(u.name, ''), COALESCE(u.phone, ''), COALESCE(u.email, ''), "
                                            "CAST(u.created_at AS CHAR), COALESCE(t.type, '') "
                                            "FROM users AS u "
                                            "LEFT JOIN types AS t ON u.type_id = t.id "
                                            "ORDER BY u.created_at DESC, u.id DESC "
                                            "LIMIT 80")
                                      .execute();

        nlohmann::json response = nlohmann::json::array();
        for (auto row : users)
        {
            const int userId = row[0].get<int>();

            mysqlx::SqlResult pets = dbManager->getSession()
                                         ->sql("SELECT id, COALESCE(pet_name, ''), COALESCE(pet_type, ''), "
                                               "COALESCE(pet_age, ''), COALESCE(pet_sex, '') "
                                               "FROM pets "
                                               "WHERE user_id = ? "
                                               "ORDER BY id DESC")
                                         .bind(userId)
                                         .execute();

            nlohmann::json petList = nlohmann::json::array();
            for (auto petRow : pets)
            {
                nlohmann::json pet;
                pet["id"] = std::to_string(petRow[0].get<int>());
                pet["name"] = petRow[1].isNull() ? "" : petRow[1].get<std::string>();
                pet["species"] = petRow[2].isNull() ? "" : petRow[2].get<std::string>();
                pet["breed"] = "";
                pet["age"] = petRow[3].isNull() ? "" : petRow[3].get<std::string>();
                pet["sex"] = petRow[4].isNull() ? "" : petRow[4].get<std::string>();
                pet["weight"] = "";
                pet["orderIds"] = nlohmann::json::array();
                petList.push_back(pet);
            }

            nlohmann::json profile;
            profile["id"] = std::to_string(userId);
            profile["ownerName"] = row[1].isNull() ? "" : row[1].get<std::string>();
            profile["phone"] = row[2].isNull() ? "" : row[2].get<std::string>();
            profile["email"] = row[3].isNull() ? "" : row[3].get<std::string>();
            profile["address"] = "";
            profile["memberLevel"] = row[5].isNull() ? "" : row[5].get<std::string>();
            profile["balance"] = 0;
            profile["note"] = row[4].isNull() ? "" : row[4].get<std::string>();
            profile["pets"] = petList;
            profile["orders"] = nlohmann::json::array();
            response.push_back(profile);
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response doctorHandler::getReservations(const crow::request &req, int doctorId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        const bool isBoss = RoleTypeUtils::userHasBossRole(dbManager, doctorId);
        const std::string sql =
            "SELECT r.id, COALESCE(u.name, ''), COALESCE(u.phone, ''), COALESCE(d.name, ''), "
            "r.pet_id, COALESCE(p.pet_name, ''), "
            "CAST(r.date AS CHAR), COALESCE(r.time_slot, ''), COALESCE(r.status, ''), "
            "CAST(r.created_at AS CHAR) "
            "FROM reaservations AS r "
            "LEFT JOIN users AS u ON r.user_id = u.id "
            "LEFT JOIN users AS d ON r.doctor_id = d.id "
            "LEFT JOIN pets AS p ON r.pet_id = p.id " +
            std::string(isBoss ? "" : "WHERE r.doctor_id = ? ") +
            "ORDER BY r.date DESC, r.created_at DESC";

        auto query = dbManager->getSession()->sql(sql);
        if (!isBoss)
        {
            query.bind(doctorId);
        }
        mysqlx::SqlResult result = query.execute();

        nlohmann::json response = nlohmann::json::array();
        for (auto row : result)
        {
            const std::string date = row[6].isNull() ? "" : row[6].get<std::string>();
            const std::string timeSlot = row[7].isNull() ? "" : row[7].get<std::string>();

            nlohmann::json item;
            item["id"] = row[0].get<int>();
            item["petId"] = row[4].isNull() ? 0 : row[4].get<int>();
            item["petName"] = row[5].isNull() || row[5].get<std::string>().empty() ? "预约记录" : row[5].get<std::string>();
            item["ownerName"] = row[1].isNull() ? "" : row[1].get<std::string>();
            item["phone"] = row[2].isNull() ? "" : row[2].get<std::string>();
            item["doctorName"] = row[3].isNull() ? "" : row[3].get<std::string>();
            item["schedule"] = date + (timeSlot.empty() ? "" : " " + timeSlot);
            item["project"] = "门诊预约";
            item["status"] = row[8].isNull() ? "待确认" : row[8].get<std::string>();
            item["createdAt"] = row[9].isNull() ? "" : row[9].get<std::string>();
            response.push_back(item);
        }

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
                                             .bind(ownerId, petId, doctorId, orderType, orderData, "待付款", orderTotalPrice)
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
                                                            "COALESCE(o.order_status, '待付款') "
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
        orderRecord["status"] = createdOrderRow[6].isNull() ? "待付款" : createdOrderRow[6].get<std::string>();
        orderRecord["orderMedicines"] = medicines;

        return ResponseHelper::success(req, orderRecord);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response doctorHandler::getOrderRecords(const crow::request &req, int doctorId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        const bool isBoss = RoleTypeUtils::userHasBossRole(dbManager, doctorId);
        const std::string sql =
            "SELECT o.id, COALESCE(p.pet_name, ''), COALESCE(u.name, ''), "
            "CAST(o.created_at AS CHAR), COALESCE(o.order_totalprice, 0), COALESCE(o.order_status, ''), "
            "(SELECT COUNT(*) FROM orderMedicines AS om WHERE om.order_id = o.id) "
            "FROM orders AS o "
            "LEFT JOIN pets AS p ON o.pet_id = p.id "
            "LEFT JOIN users AS u ON p.user_id = u.id " +
            std::string(isBoss ? "" : "WHERE o.doctor_id = ? ") +
            "ORDER BY o.created_at DESC";

        auto query = dbManager->getSession()->sql(sql);
        if (!isBoss)
        {
            query.bind(doctorId);
        }
        mysqlx::SqlResult result = query.execute();

        nlohmann::json response = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json item;
            item["id"] = std::to_string(row[0].get<int>());
            item["petName"] = row[1].isNull() ? "" : row[1].get<std::string>();
            item["ownerName"] = row[2].isNull() ? "" : row[2].get<std::string>();
            item["createdAt"] = row[3].isNull() ? "" : row[3].get<std::string>();
            item["totalFee"] = row[4].isNull() ? 0.0 : row[4].get<double>();
            item["status"] = row[5].isNull() ? "待付款" : row[5].get<std::string>();
            item["medicineCount"] = row[6].isNull() ? 0 : row[6].get<int>();
            response.push_back(item);
        }

        return ResponseHelper::success(req, response);
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
            result = dbManager->getSession()->sql("SELECT id, type_id, name, phone, email, birthday, head_image, created_at "
                                                  "FROM users "
                                                  "WHERE name LIKE ? "
                                                  "ORDER BY name ASC "
                                                  "LIMIT 20")
                         .bind(data + "%")
                         .execute();
        }
        else if (identifier == "phone")
        {
            std::string sql;
            if (data.size() == 4)
            {
                sql =
                    "SELECT DISTINCT u.id, u.type_id, u.name, u.phone, u.email, u.birthday, u.head_image, u.created_at "
                    "FROM phones AS p "
                    "JOIN users AS u ON p.user_id = u.id "
                    "WHERE p.phone_lastfour = ? "
                    "ORDER BY u.name ASC";
            }
            else
            {
                sql =
                    "SELECT DISTINCT u.id, u.type_id, u.name, u.phone, u.email, u.birthday, u.head_image, u.created_at "
                    "FROM phones AS p "
                    "JOIN users AS u ON p.user_id = u.id "
                    "WHERE p.phone = ? "
                    "LIMIT 1";
            }

            result = dbManager->getSession()->sql(sql).bind(data).execute();

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
        for (auto row : result)
        {
            nlohmann::json user;
            user["id"] = row[0].get<int>();
            user["type_id"] = row[1].get<int>();
            user["name"] = row[2].get<std::string>();
            user["phone"] = row[3].get<std::string>();
            user["email"] = row[4].get<std::string>();
            user["birthday"] = row[5].get<std::string>();
            user["head_image"] = row[6].get<std::string>();
            user["created_at"] = row[7].get<std::string>();

            response.push_back(user);
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
        std::string check_in_time_start;
        std::string check_in_time_end;

        mysqlx::SqlResult workTime_result = dbManager->getSession()
                                                ->sql("SELECT check_in_time_start, check_in_time_end FROM workTimes")
                                                .execute();

        auto work_time_row = workTime_result.fetchOne();
        if (!work_time_row)
        {
            return ResponseHelper::error(req, "未配置签到时间，请先联系管理员设置");
        }

        check_in_time_start = work_time_row[0].get<std::string>();
        check_in_time_end = work_time_row[1].get<std::string>();

        if (time <= check_in_time_start)
        {
            return ResponseHelper::error(req, "未到签到时间，请确认签到时间!!!");
        }
        else if (time >= check_in_time_end)
        {
            return ResponseHelper::error(req, "已超过签到时间，如果有特殊情况导致请与管理人员确认!!!");
        }
        else
        {
            mysqlx::SqlResult existing_result = dbManager->getSession()->sql("SELECT doctor_id, status "
                                                                             "FROM onlineDoctors "
                                                                             "WHERE doctor_id = ? "
                                                                             "LIMIT 1")
                                                    .bind(userId)
                                                    .execute();

            auto existing_row = existing_result.fetchOne();
            if (existing_row)
            {
                std::string current_status = existing_row[1].get<std::string>();

                if (current_status == "online")
                {
                    return ResponseHelper::error(req, "当前已处于签到状态，请勿重复签到!");
                }

                dbManager->getSession()->sql("UPDATE onlineDoctors "
                                             "SET date = ?, check_in_time = ?, check_out_time = NULL, status = 'online' "
                                             "WHERE doctor_id = ?")
                    .bind(date)
                    .bind(time)
                    .bind(userId)
                    .execute();
            }
            else
            {
                dbManager->getSession()->sql("INSERT INTO onlineDoctors (doctor_id, date, check_in_time, status) "
                                             "VALUES (?, ?, ?, 'online')")
                    .bind(userId)
                    .bind(date)
                    .bind(time)
                    .execute();
            }

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
        std::string check_out_time_start;
        std::string check_out_time_end;

        mysqlx::SqlResult workTime_result = dbManager->getSession()
                                                ->sql("SELECT check_out_time_start, check_out_time_end FROM workTimes")
                                                .execute();

        auto work_time_row = workTime_result.fetchOne();
        if (!work_time_row)
        {
            return ResponseHelper::error(req, "未配置签退时间，请先联系管理员设置");
        }

        check_out_time_start = work_time_row[0].get<std::string>();
        check_out_time_end = work_time_row[1].get<std::string>();

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
                              .bind(time)
                              .bind(userId)
                              .bind(date)
                              .execute();

            // 检查是否更新了记录
            if (result.getAffectedItemsCount() > 0)
            {
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
