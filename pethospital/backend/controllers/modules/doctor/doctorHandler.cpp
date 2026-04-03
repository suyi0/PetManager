#include "doctorHandler.h"
#include "../../../database/UserPhoneSync.h"
#include "RoleTypeUtils/RoleTypeUtils.h"

crow::response doctorHandler::getDoctor(const crow::request &req)
{
    try
    {
        if(!checkDbConnection())
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

crow::response doctorHandler::getDutyStatus(const crow::request &req, int &userId)
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

crow::response doctorHandler::createUser(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string username = request_body.contains("username") ? request_body["username"].get<std::string>() : "";
        std::string phone = request_body.contains("phone") ? request_body["phone"].get<std::string>() : "";
        std::string password = "123456";

        if (username.empty() || phone.empty())
        {
            return ResponseHelper::error(req, "用户名或手机号不能为空");
        }

        const int defaultUserRoleId =
            RoleTypeUtils::getRoleId(dbManager, "普通用户");
        if (defaultUserRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "普通用户角色不存在");
        }

        mysqlx::SqlResult result = dbManager->getSession()->sql("INSERT INTO users (type_id, username, password, phone) "
                                                                "VALUES (?, ?, ?, ?)")
                                       .bind(defaultUserRoleId, username, password, phone)
                                       .execute();

        if (result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::error(req, "创建用户失败");
        }

        if (!UserPhoneSync::upsertUserPhone(*dbManager, static_cast<int>(result.getAutoIncrementValue()), phone))
        {
            return ResponseHelper::system_error(req, "创建用户成功，但手机号同步失败");
        }

        return ResponseHelper::success(req, "创建用户成功");
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

            result = dbManager->getSession()->sql(sql)
                         .bind(data)
                         .execute();

            if(result.count() == 0)
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

crow::response doctorHandler::onlineDoctor(const crow::request &req, int &userId)
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
        if (work_time_row)
        {
            check_in_time_start = work_time_row[0].get<std::string>();
            check_in_time_end = work_time_row[1].get<std::string>();
        }

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

crow::response doctorHandler::offlineDoctor(const crow::request &req, int &userId)
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
        if (work_time_row)
        {
            check_out_time_start = work_time_row[0].get<std::string>();
            check_out_time_end = work_time_row[1].get<std::string>();
        }

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
