#include "adminHandler.h"
#include "../../../database/UserPhoneSync.h"
#include "../../OperationLogger/OperationLogger.h"
#include "../../../utils/RoleTypeUtils/RoleTypeUtils.h"


crow::response adminHandler::getUsers(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT u.id, u.type_id, t.type, u.name, u.phone, u.email, CAST(u.birthday AS CHAR), "
                                             "u.address_id, u.head_image, od.status "
                                             "FROM users AS u "
                                             "LEFT JOIN types AS t ON u.type_id = t.id "
                                             "LEFT JOIN onlineDoctors AS od ON od.doctor_id = u.id "
                                             "WHERE u.is_deleted = 0")
                                       .execute();

        nlohmann::json response_data = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json user_json;
            user_json["id"] = row[0].isNull() ? 0 : row[0].get<int>();
            user_json["type_id"] = row[1].isNull() ? nullptr : nlohmann::json(row[1].get<int>());
            user_json["type_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
            user_json["name"] = row[3].isNull() ? "" : clean_string(row[3].get<std::string>());
            user_json["phone"] = row[4].isNull() ? "" : clean_string(row[4].get<std::string>());
            user_json["email"] = row[5].isNull() ? "" : clean_string(row[5].get<std::string>());
            user_json["birthday"] = row[6].isNull() ? "" : row[6].get<std::string>();
            user_json["address_id"] = row[7].isNull() ? nullptr : nlohmann::json(row[7].get<int>());
            user_json["head_image"] = row[8].isNull() ? "" : clean_string(row[8].get<std::string>());
            user_json["status"] = row[9].isNull() ? "" : row[9].get<std::string>();

            response_data.push_back(user_json);
        }

        return ResponseHelper::success(req, response_data);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::operation_failed(req, "Failed to fetch data", e.what());
    }
}

crow::response adminHandler::getWorkTimeRecord(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        nlohmann::json response = nlohmann::json::array();

        boost::posix_time::ptime onlineDateTime = boost::posix_time::second_clock::local_time();
        std::string date = formatDateOnly(onlineDateTime);
        std::string time = formatTimeOnly(onlineDateTime);

        std::string check_out_time = "18:00:00"; // 默认下班时间

        // 获取系统设置的下班时间
        try
        {
            mysqlx::SqlResult workTimesTable_reslut =
                dbManager->getSession()->sql("SELECT check_out_time_end FROM workTimes").execute();

            if (workTimesTable_reslut.count() > 0)
            {
                auto row = workTimesTable_reslut.fetchOne();
                check_out_time = row[0].get<std::string>();
            }
        }
        catch (const std::exception &e)
        {
            // 如果获取失败，使用默认时间
            std::cerr << "Failed to get checkout time: " << e.what() << std::endl;
        }

        if (time <= (check_out_time + "00:00:30"))
        {
            try
            {
                mysqlx::SqlResult onlinedoctors_result =
                    dbManager->getSession()
                        ->sql("SELECT od.doctor_id, u.name, od.date, od.check_in_time, "
                              "od.check_out_time, od.status "
                              "FROM onlineDoctors AS od "
                              "JOIN users AS u ON od.doctor_id = u.id "
                              "WHERE od.date = ? "
                              "ORDER BY od.date DESC, u.name ASC")
                        .bind(date)
                        .execute();

                for (auto row : onlinedoctors_result)
                {
                    nlohmann::json doctor = nlohmann::json::object();
                    doctor["source"] = "online_doctors";
                    doctor["id"] = 0;
                    doctor["user_id"] = row[0].isNull() ? 0 : row[0].get<int>();
                    doctor["name"] = row[1].isNull() ? "" : row[1].get<std::string>();
                    doctor["date"] = row[2].isNull() ? "" : row[2].get<std::string>();
                    doctor["check_in_time"] =
                        row[3].isNull() ? "" : row[3].get<std::string>();
                    doctor["check_out_time"] =
                        row[4].isNull() ? "" : row[4].get<std::string>();
                    doctor["notes"] = "";
                    doctor["status"] = row[5].isNull() ? "" : row[5].get<std::string>();
                    doctor["created_at"] = "";
                    doctor["updated_at"] = "";
                    response.push_back(doctor);
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to fetch online doctors: " << e.what() << std::endl;
            }
        }

        try
        {
            mysqlx::SqlResult workTimeRecords_result =
                dbManager->getSession()
                    ->sql("SELECT wtr.id, wtr.doctor_id, u.name, wtr.date, "
                          "wtr.check_in_time, wtr.check_out_time, wtr.status, "
                          "wtr.notes, wtr.created_at, wtr.updated_at "
                          "FROM workTimeRecords AS wtr "
                          "JOIN users AS u ON wtr.doctor_id = u.id "
                          "ORDER BY wtr.date DESC, u.name ASC")
                    .execute();

            for (auto row : workTimeRecords_result)
            {
                nlohmann::json doctor = nlohmann::json::object();
                doctor["source"] = "work_records";
                doctor["id"] = row[0].isNull() ? 0 : row[0].get<int>();
                doctor["user_id"] = row[1].isNull() ? 0 : row[1].get<int>();
                doctor["name"] = row[2].isNull() ? "" : row[2].get<std::string>();
                doctor["date"] = row[3].isNull() ? "" : row[3].get<std::string>();
                doctor["check_in_time"] =
                    row[4].isNull() ? "" : row[4].get<std::string>();
                doctor["check_out_time"] =
                    row[5].isNull() ? "" : row[5].get<std::string>();
                doctor["status"] = row[6].isNull() ? "" : row[6].get<std::string>();
                doctor["notes"] = row[7].isNull() ? "" : row[7].get<std::string>();
                doctor["created_at"] =
                    row[8].isNull() ? "" : row[8].get<std::string>();
                doctor["updated_at"] =
                    row[9].isNull() ? "" : row[9].get<std::string>();

                response.push_back(doctor);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error querying workTimeRecords: " << e.what() << std::endl;
            return ResponseHelper::system_error(req, "Database query failed");
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response adminHandler::changeDoctorWorkTime(const crow::request &req, int &userId, const std::string &date, const std::string &identifier)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        boost::posix_time::ptime onlineDateTime = boost::posix_time::second_clock::local_time();
        std::string todayDate = formatDateOnly(onlineDateTime);
        std::string time_value;

        // 获取工作时间配置
        const std::string column_name = identifier == "check_in_time" ? "check_in_time_end" : "check_out_time_end";
        mysqlx::SqlResult workTime_result = dbManager->getSession()
                                                ->sql("SELECT " + column_name + " FROM workTimes")
                                                .execute();

        if (workTime_result.count() > 0)
        {
            mysqlx::Row row = workTime_result.fetchOne();
            time_value = row[0].get<std::string>();
        }
        else
        {
            return ResponseHelper::notFound(req, "Work time configuration not found");
        }

        // 根据日期选择不同的表进行更新
        int affected_rows = 0;
        if (todayDate == date)
        {
            // 更新今日在线医生表
            mysqlx::SqlResult result = dbManager->getSession()
                                           ->sql("UPDATE onlineDoctors SET " + identifier + " = ? WHERE doctor_id = ? AND date = ?")
                                           .bind(time_value, userId, date)
                                           .execute();

            affected_rows = result.getAffectedItemsCount();
        }
        else
        {
            // 更新历史工作时间记录表
            mysqlx::SqlResult result = dbManager->getSession()
                                           ->sql("UPDATE workTimeRecords SET " + identifier + " = ? WHERE doctor_id = ? AND date = ?")
                                           .bind(time_value, userId, date)
                                           .execute();

            affected_rows = result.getAffectedItemsCount();
        }

        if (affected_rows == 0)
        {
            return ResponseHelper::notFound(req, "No matching record found to update");
        }
        else
        {
            return ResponseHelper::success(req, "更新成功!!!");
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response adminHandler::handleDoctorStatusAction(const crow::request &req, int &userId, bool requireDoctorId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

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

        const int doctorRoleId = RoleTypeUtils::getRoleId(dbManager, "医生");
        if (doctorRoleId <= 0)
        {
            return ResponseHelper::notFound(req, "医生角色不存在");
        }

        auto doctorResult = dbManager->getSession()
                                ->sql("SELECT id FROM users WHERE id = ? AND type_id = ? LIMIT 1")
                                .bind(targetDoctorId)
                                .bind(doctorRoleId)
                                .execute();
        auto doctorRow = doctorResult.fetchOne();
        if (!doctorRow)
        {
            return ResponseHelper::notFound(req, "目标医生不存在");
        }

        boost::posix_time::ptime currentDateTime = boost::posix_time::second_clock::local_time();
        std::string todayDate = formatDateOnly(currentDateTime);
        std::string currentTime = formatTimeOnly(currentDateTime);

        auto existingResult = dbManager->getSession()
                                  ->sql("SELECT date, status FROM onlineDoctors WHERE doctor_id = ? LIMIT 1")
                                  .bind(targetDoctorId)
                                  .execute();
        auto existingRow = existingResult.fetchOne();

        if (status == "online")
        {
            if (existingRow)
            {
                const std::string currentDate = existingRow[0].isNull() ? "" : existingRow[0].get<std::string>();
                const std::string currentStatus = existingRow[1].isNull() ? "" : existingRow[1].get<std::string>();
                if (currentStatus == "online")
                {
                    return ResponseHelper::success(req, currentDate == todayDate ? "医生当前已处于上班状态" : "医生已处于上班状态，已同步为今日记录");
                }

                dbManager->getSession()
                    ->sql("UPDATE onlineDoctors "
                          "SET date = ?, check_in_time = ?, check_out_time = NULL, status = 'online' "
                          "WHERE doctor_id = ?")
                    .bind(todayDate)
                    .bind(currentTime)
                    .bind(targetDoctorId)
                    .execute();
            }
            else
            {
                dbManager->getSession()
                    ->sql("INSERT INTO onlineDoctors (doctor_id, date, check_in_time, check_out_time, status) "
                          "VALUES (?, ?, ?, NULL, 'online')")
                    .bind(targetDoctorId)
                    .bind(todayDate)
                    .bind(currentTime)
                    .execute();
            }

            return ResponseHelper::success(req, "更新成功");
        }

        if (status == "offline")
        {
            if (!existingRow)
            {
                dbManager->getSession()
                    ->sql("INSERT INTO onlineDoctors (doctor_id, date, check_in_time, check_out_time, status) "
                          "VALUES (?, ?, NULL, NULL, 'offline')")
                    .bind(targetDoctorId)
                    .bind(todayDate)
                    .execute();

                return ResponseHelper::success(req, "更新成功");
            }

            const std::string currentStatus = existingRow[1].isNull() ? "" : existingRow[1].get<std::string>();
            if (currentStatus == "offline")
            {
                return ResponseHelper::success(req, "医生当前已处于下班状态");
            }

            auto result = dbManager->getSession()
                              ->sql("UPDATE onlineDoctors "
                                    "SET date = ?, check_out_time = ?, status = 'offline' "
                                    "WHERE doctor_id = ?")
                              .bind(todayDate)
                              .bind(currentTime)
                              .bind(targetDoctorId)
                              .execute();

            if (result.getAffectedItemsCount() == 0)
            {
                return ResponseHelper::notFound(req, "未找到可更新的值班记录");
            }

            return ResponseHelper::success(req, "更新成功");
        }

        return ResponseHelper::validation(req, "Invalid status value");
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "管理", "修改医生工作状态", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response adminHandler::getLogs(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        mysqlx::SqlResult userLogs_result = dbManager->getSession()
                                                ->sql("SELECT CAST(id AS CHAR), category, user_role, operator, module, action, result, "
                                                      "CAST(created_at AS CHAR), summary, details, source "
                                                      "FROM user_operations "
                                                      "ORDER BY created_at DESC")
                                                .execute();

        mysqlx::SqlResult systemLogs_result = dbManager->getSession()
                                                  ->sql("SELECT CAST(id AS CHAR), category, system_role, operator, module, action, result, "
                                                        "CAST(created_at AS CHAR), summary, details, source "
                                                        "FROM system_operations "
                                                        "ORDER BY created_at DESC")
                                                  .execute();

        nlohmann::json response;
        response["userLogs"] = nlohmann::json::array();
        response["systemLogs"] = nlohmann::json::array();

        for (auto row : userLogs_result)
        {
            nlohmann::json log;
            log["id"] = row[0].isNull() ? "" : row[0].get<std::string>();
            log["category"] = row[1].isNull() ? "" : row[1].get<std::string>();
            log["userRole"] = row[2].isNull()
                                  ? nlohmann::json(nullptr)
                                  : nlohmann::json(row[2].get<std::string>());
            log["operator"] = row[3].isNull() ? "" : clean_string(row[3].get<std::string>());
            log["module"] = row[4].isNull() ? "" : clean_string(row[4].get<std::string>());
            log["action"] = row[5].isNull() ? "" : clean_string(row[5].get<std::string>());
            log["result"] = row[6].isNull() ? "" : row[6].get<std::string>();
            log["time"] = row[7].isNull() ? "" : row[7].get<std::string>();
            log["summary"] = row[8].isNull() ? "" : clean_string(row[8].get<std::string>());
            log["details"] = row[9].isNull() ? "" : clean_string(row[9].get<std::string>());
            log["source"] = row[10].isNull() ? "" : row[10].get<std::string>();

            response["userLogs"].push_back(log);
        }

        for (auto row : systemLogs_result)
        {
            nlohmann::json log;
            log["id"] = row[0].isNull() ? "" : row[0].get<std::string>();
            log["category"] = row[1].isNull() ? "" : row[1].get<std::string>();
            log["systemRole"] = row[2].isNull()
                                    ? nlohmann::json(nullptr)
                                    : nlohmann::json(row[2].get<std::string>());
            log["operator"] = row[3].isNull() ? "" : clean_string(row[3].get<std::string>());
            log["module"] = row[4].isNull() ? "" : clean_string(row[4].get<std::string>());
            log["action"] = row[5].isNull() ? "" : clean_string(row[5].get<std::string>());
            log["result"] = row[6].isNull() ? "" : row[6].get<std::string>();
            log["time"] = row[7].isNull() ? "" : row[7].get<std::string>();
            log["summary"] = row[8].isNull() ? "" : clean_string(row[8].get<std::string>());
            log["details"] = row[9].isNull() ? "" : clean_string(row[9].get<std::string>());
            log["source"] = row[10].isNull() ? "" : row[10].get<std::string>();

            response["systemLogs"].push_back(log);
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response adminHandler::getSalaryManagementData(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        const int normalUserRoleId = RoleTypeUtils::getRoleId(dbManager, "普通用户");
        if (normalUserRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "普通用户角色不存在");
        }

        mysqlx::SqlResult employeesResult = dbManager->getSession()
                                               ->sql("SELECT u.id, u.type_id, t.type, u.name, u.phone, u.email, "
                                                     "COALESCE(s.base_salary, 0), COALESCE(s.PA_Award, 0), "
                                                     "COALESCE(s.PB_Award, 0), COALESCE(s.total_salary, 0), "
                                                     "CAST(s.updated_at AS CHAR) "
                                                     "FROM users AS u "
                                                     "LEFT JOIN types AS t ON u.type_id = t.id "
                                                     "LEFT JOIN salary AS s ON s.user_id = u.id "
                                                     "WHERE u.type_id <> ? AND u.is_deleted = 0 "
                                                     "ORDER BY COALESCE(s.total_salary, 0) DESC, u.id ASC")
                                               .bind(normalUserRoleId)
                                               .execute();

        mysqlx::SqlResult monthlyRecordsResult = dbManager->getSession()
                                                    ->sql("SELECT CAST(id AS CHAR), salesCount, costCount, profitCount, "
                                                          "CAST(created_at AS CHAR) "
                                                          "FROM salaryRecord "
                                                          "WHERE record_type = 'month' "
                                                          "ORDER BY created_at DESC "
                                                          "LIMIT 6")
                                                    .execute();

        mysqlx::SqlResult dailyRecordsResult = dbManager->getSession()
                                                  ->sql("SELECT CAST(id AS CHAR), salesCount, costCount, profitCount, "
                                                        "CAST(created_at AS CHAR) "
                                                        "FROM monthlySalaryRecord "
                                                        "ORDER BY created_at DESC "
                                                        "LIMIT 10")
                                                  .execute();

        nlohmann::json response;
        response["employees"] = nlohmann::json::array();
        response["monthlyRecords"] = nlohmann::json::array();
        response["dailyRecords"] = nlohmann::json::array();

        double totalPayroll = 0.0;
        int employeeCount = 0;

        for (auto row : employeesResult)
        {
            nlohmann::json employee;
            const double baseSalary = row[6].isNull() ? 0.0 : row[6].get<double>();
            const double paAward = row[7].isNull() ? 0.0 : row[7].get<double>();
            const double pbAward = row[8].isNull() ? 0.0 : row[8].get<double>();
            const double totalSalary = row[9].isNull() ? 0.0 : row[9].get<double>();

            employee["id"] = row[0].isNull() ? 0 : row[0].get<int>();
            employee["type_id"] = row[1].isNull() ? nullptr : nlohmann::json(row[1].get<int>());
            employee["type_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
            employee["name"] = row[3].isNull() ? "" : clean_string(row[3].get<std::string>());
            employee["phone"] = row[4].isNull() ? "" : clean_string(row[4].get<std::string>());
            employee["email"] = row[5].isNull() ? "" : clean_string(row[5].get<std::string>());
            employee["base_salary"] = baseSalary;
            employee["pa_award"] = paAward;
            employee["pb_award"] = pbAward;
            employee["total_salary"] = totalSalary;
            employee["updated_at"] = row[10].isNull() ? "" : row[10].get<std::string>();

            totalPayroll += totalSalary;
            employeeCount += 1;
            response["employees"].push_back(employee);
        }

        for (auto row : monthlyRecordsResult)
        {
            nlohmann::json record;
            record["id"] = row[0].isNull() ? "" : row[0].get<std::string>();
            record["salesCount"] = row[1].isNull() ? 0.0 : row[1].get<double>();
            record["costCount"] = row[2].isNull() ? 0.0 : row[2].get<double>();
            record["profitCount"] = row[3].isNull() ? 0.0 : row[3].get<double>();
            record["created_at"] = row[4].isNull() ? "" : row[4].get<std::string>();
            response["monthlyRecords"].push_back(record);
        }

        for (auto row : dailyRecordsResult)
        {
            nlohmann::json record;
            record["id"] = row[0].isNull() ? "" : row[0].get<std::string>();
            record["salesCount"] = row[1].isNull() ? 0.0 : row[1].get<double>();
            record["costCount"] = row[2].isNull() ? 0.0 : row[2].get<double>();
            record["profitCount"] = row[3].isNull() ? 0.0 : row[3].get<double>();
            record["created_at"] = row[4].isNull() ? "" : row[4].get<std::string>();
            response["dailyRecords"].push_back(record);
        }

        double salesCount = financer.calculateSalesCount();
        double costCount = financer.calculateCostCount();

        response["summary"] = {
            {"employeeCount", employeeCount},
            {"monthlyPayroll", totalPayroll},
            {"todayCost", costCount},
            {"todayProfit", salesCount - costCount}};

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response adminHandler::changeSalary(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userId = request_body.value("userId", 0);
        double baseSalary = request_body.value("baseSalary", 0.0);
        double paAward = request_body.value("paAward", 0.0);
        double pbAward = request_body.value("pbAward", 0.0);
        double totalSalary = baseSalary + paAward + pbAward;

        if (userId == 0)
        {
            return ResponseHelper::unavailable(req, "用户ID不能为空");
        }
        if (baseSalary < 0 || paAward < 0 || pbAward < 0)
        {
            return ResponseHelper::unavailable(req, "工资项不能小于0");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            mysqlx::SqlResult salaryRowResult = session->sql("SELECT id FROM salary WHERE user_id = ? LIMIT 1")
                                                    .bind(userId)
                                                    .execute();

            mysqlx::Row salaryRow = salaryRowResult.fetchOne();

            if (salaryRow && !salaryRow[0].isNull())
            {
                mysqlx::SqlResult updateResult = session->sql("UPDATE salary SET base_salary = ?, PA_Award = ?, PB_Award = ?, total_salary = ? WHERE user_id = ?")
                                                     .bind(baseSalary, paAward, pbAward, totalSalary, userId)
                                                     .execute();
                if (updateResult.getAffectedItemsCount() != 1)
                {
                    session->sql("ROLLBACK").execute();
                    return ResponseHelper::operation_failed(req, "工资修改失败");
                }
            }
            else
            {
                mysqlx::SqlResult insertResult = session->sql("INSERT INTO salary (user_id, base_salary, PA_Award, PB_Award, total_salary) "
                                                              "VALUES (?, ?, ?, ?, ?)")
                                                     .bind(userId, baseSalary, paAward, pbAward, totalSalary)
                                                     .execute();

                if (insertResult.getAffectedItemsCount() != 1)
                {
                    session->sql("ROLLBACK").execute();
                    return ResponseHelper::operation_failed(req, "工资创建失败");
                }

                const int salaryId = static_cast<int>(insertResult.getAutoIncrementValue());
                mysqlx::SqlResult userUpdateResult = session->sql("UPDATE users SET salary_id = ? WHERE id = ?")
                                                         .bind(salaryId, userId)
                                                         .execute();
                if (userUpdateResult.getAffectedItemsCount() != 1)
                {
                    session->sql("ROLLBACK").execute();
                    return ResponseHelper::operation_failed(req, "工资关联用户失败");
                }
            }

            session->sql("COMMIT").execute();
        }
        catch (...)
        {
            session->sql("ROLLBACK").execute();
            throw;
        }

        return ResponseHelper::success(req, "工资修改成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

int adminHandler::calculateUserCount()
{
    try
    {
        if (!checkDbConnection())
        {
            return -1; // 或者抛出异常，具体取决于你的错误处理策略
        }

        int userCount = dbManager->getSession()
                            ->sql("SELECT COUNT(*) FROM users")
                            .execute()
                            .fetchOne()[0]
                            .get<int>();
        return userCount;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to get userCount: " + std::string(e.what()));
    }
}

int adminHandler::calculateOnlineDoctorCount()
{
    try
    {
        if (!checkDbConnection())
        {
            return -1;
        }

        int OnlineDoctorCount = dbManager->getSession()
                                    ->sql("SELECT COUNT(*) FROM onlineDoctors WHERE status = 'online'")
                                    .execute()
                                    .fetchOne()[0]
                                    .get<int>();
        return OnlineDoctorCount;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to get onlineDoctorCount: " + std::string(e.what()));
    }
}

int adminHandler::calculateLogsCount()
{
    try
    {
        if (!checkDbConnection())
        {
            return -1;
        }

        const int logsCount = dbManager->getSession()
                                  ->sql("SELECT "
                                        "(SELECT COUNT(*) FROM system_operations) + "
                                        "(SELECT COUNT(*) FROM user_operations)")
                                  .execute()
                                  .fetchOne()[0]
                                  .get<int>();
        return logsCount;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to get logsCount: " + std::string(e.what()));
    }
}

crow::response adminHandler::getAllRecord(const crow::request &req, int &userId, int batch_size, int offset)
{
    try
    {
        if(!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "订单", "获取订单详情", "database connection failed");
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        // 使用 JOIN 一次性查询，避免 N+1 查询问题
        mysqlx::SqlResult orders_result = dbManager->getSession()->sql(
                                                                     "SELECT o.id, o.pet_id, p.pet_name, o.doctor_id, o.order_type, "
                                                                     "o.order_data, o.order_status, o.order_totalprice, o.created_at "
                                                                     "FROM orders as o "
                                                                     "JOIN pets as p ON o.pet_id = p.id "
                                                                     "WHERE p.user_id = ? "
                                                                     "ORDER BY o.order_data DESC, o.created_at DESC "
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
            order["order_data"] = order_row[5].get<std::string>();
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
