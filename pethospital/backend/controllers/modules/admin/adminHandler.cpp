#include "adminHandler.h"
#include "../../../database/UserPhoneSync.h"
#include "../../../utils/RoleTypeUtils/RoleTypeUtils.h"

crow::response adminHandler::getWorkTimeRecord(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::system_error(req);
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

crow::response adminHandler::createUser(const crow::request& req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string name = request_body.value("name", "");
        std::string phone = request_body.value("phone", "");
        std::string email = request_body.value("email", "");
        std::string password = request_body.value("password", "");
        std::string birthday = request_body.value("birthday", "1970-01-01");
        int address_id = request_body.value("address_id", 0);
        std::string head_image = request_body.value("head_image", "");

        if (name.empty())
        {
            return ResponseHelper::validation(req, "用户名不能为空");
        }

        if (phone.empty() && email.empty())
        {
            return ResponseHelper::validation(req, "手机号和邮箱至少填写一项");
        }

        if (password.empty())
        {
            password = "123456";
        }

        if (!phone.empty())
        {
            mysqlx::SqlResult phone_result = dbManager->getSession()
                ->sql("SELECT id FROM users WHERE phone = ?")
                .bind(phone)
                .execute();

            if (phone_result.count() > 0)
            {
                return ResponseHelper::validation(req, "手机号已存在");
            }
        }

        if (!email.empty())
        {
            mysqlx::SqlResult email_result = dbManager->getSession()
                ->sql("SELECT id FROM users WHERE email = ?")
                .bind(email)
                .execute();

            if (email_result.count() > 0)
            {
                return ResponseHelper::validation(req, "邮箱已存在");
            }
        }

        const std::string hashed_password = hash_password(password);

        const int defaultUserRoleId =
            RoleTypeUtils::getRoleId(dbManager, "普通用户");
        if (defaultUserRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "普通用户角色不存在");
        }

        mysqlx::SqlResult result = dbManager->getSession()
            ->sql("INSERT INTO users (type_id, name, phone, password, email, birthday, address_id, head_image) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?)")
            .bind(defaultUserRoleId, name, phone, hashed_password, email, birthday, address_id, head_image)
            .execute();
        
        if(result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::system_error(req, "创建失败");
        }

        if (!UserPhoneSync::upsertUserPhone(*dbManager, static_cast<int>(result.getAutoIncrementValue()), phone))
        {
            return ResponseHelper::system_error(req, "用户已创建，但手机号同步失败");
        }

        nlohmann::json payload;
        payload["success"] = true;
        payload["message"] = "创建成功";
        payload["data"] = {
            {"id", result.getAutoIncrementValue()},
            {"type_id", defaultUserRoleId},
            {"type_name", "普通用户"},
            {"name", name},
            {"phone", phone},
            {"email", email},
            {"birthday", birthday},
            {"address_id", address_id},
            {"head_image", head_image},
        };

        return ResponseHelper::created(req, payload);

    }
    catch(const std::exception& e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
    
}

crow::response adminHandler::deleteUser(const crow::request &req, int &userId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::system_error(req);
        }

        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userID = request_body.value("user_id", 0);
        if(userID == 0)
        {
            return ResponseHelper::validation(req, "用户ID不能为空");
        }

        if (userID == userId)
        {
            return ResponseHelper::validation(req, "不能删除当前登录的超级管理员");
        }

        mysqlx::SqlResult target_result = dbManager->getSession()
            ->sql("SELECT type_id FROM users WHERE id = ?")
            .bind(userID)
            .execute();

        if (target_result.count() == 0)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        mysqlx::Row target_row = target_result.fetchOne();
        const int target_type = target_row[0].isNull() ? 0 : target_row[0].get<int>();
        const std::string target_role_name =
            RoleTypeUtils::getRoleName(dbManager, target_type);
        if (target_role_name != "普通用户")
        {
            return ResponseHelper::unavailable(req, "这里只能删除普通用户");
        }

        mysqlx::SqlResult result = dbManager->getSession()
            ->sql("DELETE FROM users WHERE id = ? AND type_id = ?")
            .bind(userID, target_type)
            .execute();

        if(result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        return ResponseHelper::success(req, "删除成功");
    }
    catch(const std::exception& e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
    
}

crow::response adminHandler::createDoctor(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userId = request_body.value("user_id", 0);

        if(userId == 0)
        {
            return ResponseHelper::unavailable(req, "用户ID不能为空");
        }

        const int doctorRoleId = RoleTypeUtils::getRoleId(dbManager, "医生");
        if (doctorRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "医生角色不存在");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                    ->sql("UPDATE users SET type_id = ? WHERE id = ?")
                                    .bind(doctorRoleId, userId)
                                    .execute();

        if (result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req);
        }

        boost::posix_time::ptime currentDateTime = boost::posix_time::second_clock::local_time();
        std::string todayDate = formatDateOnly(currentDateTime);

        mysqlx::SqlResult onlineDoctorResult = dbManager->getSession()
            ->sql("SELECT doctor_id FROM onlineDoctors WHERE doctor_id = ? LIMIT 1")
            .bind(userId)
            .execute();

        if (onlineDoctorResult.count() == 0)
        {
            dbManager->getSession()
                ->sql("INSERT INTO onlineDoctors (doctor_id, date, check_in_time, check_out_time, status) "
                      "VALUES (?, ?, NULL, NULL, 'offline')")
                .bind(userId)
                .bind(todayDate)
                .execute();
        }

        return ResponseHelper::success(req, "给予权限成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response adminHandler::deleteDoctor(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userId = request_body.value("user_id", 0);

        if(userId == 0)
        {
            return ResponseHelper::unavailable(req, "用户ID不能为空");
        }

        const int defaultUserRoleId =
            RoleTypeUtils::getRoleId(dbManager, "普通用户");
        if (defaultUserRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "普通用户角色不存在");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                    ->sql("UPDATE users SET type_id = ? WHERE id = ?")
                                    .bind(defaultUserRoleId, userId)
                                    .execute();

        if (result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req);
        }

        return ResponseHelper::success(req, "删除权限成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response adminHandler::createWarehouserManager(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userId = request_body.value("user_id", 0);

        if(userId == 0)
        {
            return ResponseHelper::unavailable(req, "用户ID不能为空");
        }

        const int warehouseRoleId =
            RoleTypeUtils::getRoleId(dbManager, "仓库管理员");
        if (warehouseRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "仓库管理员角色不存在");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                    ->sql("UPDATE users SET type_id = ? WHERE id = ?")
                                    .bind(warehouseRoleId, userId)
                                    .execute();

        if (result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req);
        }

        return ResponseHelper::success(req, "给予权限成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response adminHandler::deleteWarehouserManager(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userId = request_body.value("user_id", 0);

        if(userId == 0)
        {
            return ResponseHelper::unavailable(req, "用户ID不能为空");
        }

        const int defaultUserRoleId =
            RoleTypeUtils::getRoleId(dbManager, "普通用户");
        if (defaultUserRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "普通用户角色不存在");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                    ->sql("UPDATE users SET type_id = ? WHERE id = ?")
                                    .bind(defaultUserRoleId, userId)
                                    .execute();
        
        if (result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req);
        }

        return ResponseHelper::success(req, "删除权限成功");
    }
    catch(const std::exception& e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
    
}

crow::response adminHandler::changeDoctorWorkTime(const crow::request &req, int &userId, const std::string &date, const std::string &identifier)
{
    try
    {
        if(!checkDbConnection())
        {
            return ResponseHelper::system_error(req);
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
