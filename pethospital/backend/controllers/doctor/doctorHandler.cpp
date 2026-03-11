#include "doctorHandler.h"

crow::response doctorHandler::getDoctor(const crow::request &req)
{
    try
    {
        crow::response res;
        // auto request_body_opt = validateRequest(req, res);
        // if (!request_body_opt) return res;
        // auto& request_body = request_body_opt.value();

        mysqlx::SqlResult result = dbManager->getSession()->sql("SELECT u.name, u.user_specialty, u.introduction, u.level "
                                                                "FROM users as u "
                                                                "JOIN onlinedoctors as od ON u.id = od.doctor_id "
                                                                "JOIN types as t ON u.type_id = t.id "
                                                                "WHERE t.type = 'doctor' AND od.status = 'online'")
                                                            .execute();

        nlohmann::json response = nlohmann::json::array();

        for (auto row : result)
        {
            nlohmann::json doctor;
            doctor["name"] = row[0].get<std::string>();
            doctor["specialty"] = row[1].get<std::string>();
            doctor["introduction"] = row[2].get<std::string>();
            doctor["level"] = row[3].get<std::string>();
            response.push_back(doctor);
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response doctorHandler::getUserList(const crow::request &req, const std::string &name)
{

    return ResponseHelper::success(req, "");
}
crow::response doctorHandler::onlineDoctor(const crow::request &req, int &userId)
{
    try
    {
        if (!dbManager || !dbManager->getSession() || !dbManager->getSchema())
        {
            return ResponseHelper::system_error(req);
        }

        boost::posix_time::ptime onlineDateTime = boost::posix_time::second_clock::local_time();
        std::string date = formatDateOnly(onlineDateTime);
        std::string time = formatTimeOnly(onlineDateTime);
        std::string check_in_time_start;
        std::string check_in_time_end;

        mysqlx::Table workTime_table = dbManager->getSchema()->getTable("workTimes");
        mysqlx::RowResult workTime_result = workTime_table.select("check_in_time_start", "check_in_time_end").execute();

        if (workTime_result.count() > 0)
        {
            auto row = workTime_result.fetchOne();
            check_in_time_start = row[0].get<std::string>();
            check_in_time_end = row[1].get<std::string>();
        }

        if (time <= check_in_time_start)
        {
            return ResponseHelper::error(req, "未到签到时间，请确认签到时间!!!");
        }
        else if(time >= check_in_time_end)
        {
            return ResponseHelper::error(req, "已超过签到时间，如果有特殊情况导致请与管理人员确认!!!");
        }
        else
        {
            mysqlx::Table doctor_table = dbManager->getSchema()->getTable("onlinedoctors");
            doctor_table.insert("doctor_id", "work_date", "check_in_time", "status")
                .values(userId, date, time, "online")
                .execute();
    
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
        if (!dbManager || !dbManager->getSession() || !dbManager->getSchema())
        {
            return ResponseHelper::system_error(req);
        }

        boost::posix_time::ptime offlineDateTime = boost::posix_time::microsec_clock::local_time();
        std::string date = formatDateOnly(offlineDateTime);
        std::string time = formatTimeOnly(offlineDateTime);
        std::string check_out_time_start;
        std::string check_out_time_end;

        mysqlx::Table workTime_table = dbManager->getSchema()->getTable("workTimes");
        mysqlx::RowResult workTime_result = workTime_table.select("check_out_time_start", "check_out_time_end").execute();

        if (workTime_result.count() > 0)
        {
            auto row = workTime_result.fetchOne();
            check_out_time_start = row[0].get<std::string>();
            check_out_time_end = row[1].get<std::string>();
        }

        if(time <= check_out_time_start)
        {
            return ResponseHelper::error(req, "未到签退时间，如要提前签退请与管理人员确认!!!");
        }
        else if(time >= check_out_time_end)
        {
            return ResponseHelper::error(req, "已超过签退时间，如果有特殊情况导致请与管理人员确认!!!");
        }
        else
        {
            mysqlx::Table doctorTable = dbManager->getSchema()->getTable("onlinedoctors");
            mysqlx::TableUpdate updateOp = doctorTable.update();
    
            updateOp.set("check_out_time", time)
                .set("status", "offline");
    
            mysqlx::Result result = updateOp.where("doctor_id = :doctor_id")
                                        .bind("doctor_id", userId)
                                        .bind("today", date)
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

