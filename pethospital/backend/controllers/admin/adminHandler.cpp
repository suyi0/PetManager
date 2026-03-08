#include "adminHandler.h"

crow::response adminHandler::getWorkTimeRecord(const crow::request &req)
{
    try
    {
        if(!checkDbConnection())
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
            mysqlx::Table workTimesTable = dbManager->getSchema()->getTable("workTimes");
            mysqlx::RowResult workTimesTable_reslut = workTimesTable.select("check_out_time_end").execute();

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
                mysqlx::SqlResult onlinedoctors_result = dbManager->getSession()->sql("SELECT od.doctor_id, u.name, od.date, od.check_in_time, od.check_out_time, od.status, od.created_at, od.updated_at "
                                                                                      "FROM onlinedoctors as od "
                                                                                      "JOIN users as u ON od.doctor_id = u.id "
                                                                                      "WHERE od.date = ? "
                                                                                      "ORDER BY od.date DESC, u.name ASC")
                                                             .bind(date)
                                                             .execute();

                for (auto row : onlinedoctors_result)
                {
                    nlohmann::json doctor = nlohmann::json::object();
                    doctor["source"] = "online_doctors";
                    doctor["id"] = 0;
                    doctor["user_id"] = row[0].get<int>();
                    doctor["name"] = row[1].get<std::string>();
                    doctor["date"] = row[2].get<std::string>();
                    doctor["check_in_time"] = row[3].get<std::string>();
                    doctor["check_out_time"] = row[4].get<std::string>();
                    doctor["notes"] = ""; // 工作记录通常没有备注
                    doctor["status"] = row[5].get<std::string>();
                    doctor["created_at"] = row[6].get<std::string>();
                    doctor["updated_at"] = row[7].get<std::string>();
                    response.push_back(doctor);
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error querying onlinedoctors: " << e.what() << std::endl;
            }
        }

        try
        {
            mysqlx::SqlResult workTimeRecords_result = dbManager->getSession()->sql("SELECT wtr.id, wtr.user_id, u.name, wtr.date, wtr.check_in_time, wtr.check_out_time, wtr.status, wtr.notes, wtr.created_at, wtr.updated_at "
                                                                                    "FROM workTimeRecords as wtr "
                                                                                    "JOIN users as u ON wtr.user_id = u.id "
                                                                                    "ORDER BY wtr.date DESC, u.name ASC")
                                                                            .execute();

            for (auto row : workTimeRecords_result)
            {
                nlohmann::json doctor = nlohmann::json::object();
                doctor["source"] = "work_records";
                doctor["id"] = row[0].get<int>();
                doctor["user_id"] = row[1].get<int>();
                doctor["name"] = row[2].get<std::string>();
                doctor["date"] = row[3].get<std::string>();
                doctor["check_in_time"] = row[4].get<std::string>();
                doctor["check_out_time"] = row[5].get<std::string>();
                doctor["status"] = row[6].get<std::string>();
                doctor["notes"] = row[7].get<std::string>();
                doctor["created_at"] = row[8].get<std::string>();
                doctor["updated_at"] = row[9].get<std::string>();
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

crow::response adminHandler::createDoctor(const crow::request &req, int &userId)
{
    try
    {
        if(!checkDbConnection())
        {
            return ResponseHelper::system_error(req);
        }

        mysqlx::Table users_table = dbManager->getSchema()->getTable("users");
        mysqlx::TableUpdate update_op = users_table.update();

        update_op.set("type_id", 2);

        mysqlx::Result result = update_op.where("id = :id")
                                    .bind("id", userId)
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

crow::response adminHandler::deleteDoctor(const crow::request &req, int &userId)
{
    try
    {
        if(!checkDbConnection())
        {
            return ResponseHelper::system_error(req);
        }

        mysqlx::Table users_table = dbManager->getSchema()->getTable("users");
        mysqlx::TableUpdate update_op = users_table.update();

        update_op.set("type_id", 3);

        mysqlx::Result result = update_op.where("id = :id")
                                    .bind("id", userId)
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

crow::response adminHandler::changeDoctorWorkTime(const crow::request &req, int &userId, std::string &date, std::string &identifier)
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
        mysqlx::Table workTime_table = dbManager->getSchema()->getTable("workTimes");
        mysqlx::RowResult workTime_result = workTime_table.select(identifier == "check_in_time" ? "check_in_time_end" : "check_out_time_end")
                                                .execute();

        if (workTime_result.count() > 0)
        {
            mysqlx::Row row = workTime_result.fetchOne();
            time_value = row[0].get<std::string>();
        }
        else
        {
            return ResponseHelper::custom(req, 404, "Work time configuration not found");
        }

        // 根据日期选择不同的表进行更新
        int affected_rows = 0;
        if (todayDate == date)
        {
            // 更新今日在线医生表
            mysqlx::Table onlineDoctors_table = dbManager->getSchema()->getTable("onlineDoctors");
            mysqlx::TableUpdate update_op = onlineDoctors_table.update();

            update_op.set(identifier, time_value);

            mysqlx::Result result = update_op.where("doctor_id = :doctor_id AND date = :date")
                                        .bind("doctor_id", userId)
                                        .bind("date", date)
                                        .execute();

            affected_rows = result.getAffectedItemsCount();
        }
        else
        {
            // 更新历史工作时间记录表
            mysqlx::Table workTimeRecords_table = dbManager->getSchema()->getTable("workTimeRecords");
            mysqlx::TableUpdate update_op = workTimeRecords_table.update();

            update_op.set(identifier, time_value);

            mysqlx::Result result = update_op.where("doctor_id = :doctor_id AND date = :date")
                                        .bind("doctor_id", userId)
                                        .bind("date", date)
                                        .execute();

            affected_rows = result.getAffectedItemsCount();
        }

        if (affected_rows == 0)
        {
            return ResponseHelper::custom(req, 404, "No matching record found to update");
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
