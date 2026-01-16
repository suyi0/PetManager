#include "ReservationHandler.h"
#include <iostream>

// 创建预约表记录接口
crow::response ReservationHandler::createReservation(const crow::request &req, int user_id, std::string name, std::string email, std::string phone, int doctor_id, std::string date, std::string time_slot, std::string status, std::string creation_time)
{
    try
    {
        // 检查数据库连接是否存在
        if (!dbManager || !dbManager->getSchema() || !dbManager->getSession())
        {
            return ResponseHelper::custom(req, 500, "error: Database connection not available");
        }

        // 获取表
        mysqlx::Table users_table = dbManager->getSchema()->getTable("users");
        mysqlx::Table reservation_table = dbManager->getSchema()->getTable("reaservations");

        mysqlx::RowResult users_result;
        if (!email.empty())
        {
            users_result = users_table.select("id")
                               .where("email = :email")
                               .bind("email", email)
                               .execute();
        }
        else if (!phone.empty())
        {
            users_result = users_table.select("id")
                               .where("phone = :phone")
                               .bind("phone", phone)
                               .execute();
        }
        else
        {
            return crow::response(500, "error: Either email or phone must be provided");
        }

        auto user_row = users_result.fetchOne();
        // 如果用户不存在，返回错误
        if (!user_row)
        {
            return crow::response(404, "error: User not found");
        }
        user_id = user_row[0].get<int>();

        // 检查必要字段是否存在
        
        if (user_id != 0 && doctor_id != 0 && !date.empty() && !time_slot.empty())
        {
            // 创建数据库插入操作
            mysqlx::TableInsert insert_op = reservation_table.insert("user_id", "doctor_id", "date", "time_slot", "status", "creation_time");

            // 插入数据库
            try
            {
                insert_op.values(user_id, doctor_id, date, time_slot, status, creation_time).execute();
            }
            catch (const mysqlx::Error &e)
            {
                std::cerr << "Database error: " << e.what() << std::endl;
                return crow::response(500, "error: Database operation failed, details: " + std::string(e.what()) + "\"}");
            }

            // 返回成功响应
            nlohmann::json response;
            response["success"] = true;
            response["message"] = "预约成功";
            return ResponseHelper::success(req, response);
        }
        else
        {

            return ResponseHelper::custom(req, 400, "error: Missing required fields");
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::custom(req, 500, "error: Failed to save reservation, details: " + std::string(e.what()) + "\"}");
    }
}

//  获得预约记录接口
crow::response ReservationHandler::getReservations(const crow::request &req, int user_id)
{
    try
    {
        // 检查数据库连接是否存在
        if (!dbManager || !dbManager->getSchema() || !dbManager->getSession())
        {
            return ResponseHelper::custom(req, 500, "error: Database connection not available");
        }

        try
        {
            // 获取预约表
            mysqlx::Table reservation_table = dbManager->getSchema()->getTable("reaservations");

            // 查询该用户的所有预约记录
            mysqlx::RowResult result = reservation_table.select("id", "user_id", "doctor_id", "date", "time_slot", "status", "creation_time")
                                           .where("user_id = :user_id")
                                           .orderBy("date DESC")
                                           .bind(":user_id", user_id)
                                           .execute();

            // 构建响应数据
            nlohmann::json response_data = nlohmann::json::array();
            for (auto row : result)
            {
                nlohmann::json record;
                record["id"] = row[0].get<int>();
                record["user_id"] = row[1].get<int>();
                record["doctor_id"] = row[2].get<int>();
                record["date"] = row[3].get<std::string>();
                record["time_slot"] = row[4].get<std::string>();
                record["status"] = row[5].get<std::string>();
                record["creation_time"] = row[6].get<std::string>();
                response_data.push_back(record);
            }

            // 返回成功响应
            nlohmann::json response;
            response["success"] = true;
            response["data"] = response_data;
            return ResponseHelper::success(req, response);
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "Database error: " << e.what() << std::endl;
            return ResponseHelper::custom(req, 500, "error: Database operation failed, details: " + std::string(e.what()) + "\"}");
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::custom(req, 500, "error: Failed to fetch reservations, details: " + std::string(e.what()) + "\"}");
    }
}

// 更新预约记录接口
crow::response ReservationHandler::updateReservation(const crow::request &req, int id)
{
    try
    {
        // 解析请求体中的 JSON 数据
        nlohmann::json request_body;
        crow::response res;
        if (!parseJsonBody(req, res, request_body))
        {
            return ResponseHelper::custom(req, 400, "error: Invalid JSON");
        }

        // 检查数据库连接是否存在
        if (!dbManager || !dbManager->getSchema() || !dbManager->getSession())
        {
            return ResponseHelper::custom(req, 500, "error: Database connection not available");
        }

        // 获取预约表
        mysqlx::Table reservation_table = dbManager->getSchema()->getTable("reaservations");

        // 更新指定ID的预约记录
        mysqlx::TableUpdate update_op = reservation_table.update();

        // 根据请求体中的字段更新相应的列
        if (request_body.find("date") != request_body.end())
        {
            update_op.set("date", request_body["date"].get<std::string>());
        }
        if (request_body.find("time_slot") != request_body.end())
        {
            update_op.set("time_slot", request_body["time_slot"].get<std::string>());
        }
        if (request_body.find("status") != request_body.end())
        {
            update_op.set("status", request_body["status"].get<std::string>());
        }

        mysqlx::Result result = update_op.where("id = :id").bind("id", id).execute();

        // 检查是否有记录被更新
        if (result.getAffectedItemsCount() > 0)
        {
            // 返回成功响应
            nlohmann::json response;
            response["success"] = true;
            response["message"] = "预约记录更新成功";
            return ResponseHelper::success(req, response);
        }
        else
        {
            // 没有找到对应的记录
            nlohmann::json response;
            response["success"] = false;
            response["error"] = "未找到指定的预约记录";
            return ResponseHelper::custom(req, 404, response);
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::custom(req, 500, "error: Failed to update reservation, details: " + std::string(e.what()) + "\"}");
    }
}

// 获取预约时间表数据接口
nlohmann::json ReservationHandler::getReservationData()
{
    // 这里应该生成并返回时间表
    Reservate r;
    auto schedule = r.generateSchedule();

    nlohmann::json response;
    response["success"] = true;
    response["data"] = schedule;
    return response;
}

// 获取医生列表接口
crow::response ReservationHandler::getDoctorList(const crow::request &req)
{
    try
    {
        // 检查数据库连接是否存在
        if (!dbManager || !dbManager->getSchema() || !dbManager->getSession())
        {
            nlohmann::json error_response;
            error_response["success"] = false;
            error_response["error"] = "Database connection not available";
            return ResponseHelper::system_error(req);
        }

        try
        {
            // 获取在线医生表
            mysqlx::Table online_doctors_table = dbManager->getSchema()->getTable("onlineDoctors");

            // 查询所有在线医生
            mysqlx::RowResult result = online_doctors_table.select("doctor_id", "name", "specialty").execute();

            // 构建医生列表数据
            nlohmann::json doctor_list = nlohmann::json::array();
            for (auto row : result)
            {
                nlohmann::json doctor;
                doctor["doctor_id"] = row[0].get<int>();
                doctor["name"] = row[1].get<std::string>();
                doctor["specialty"] = row[2].get<std::string>();

                // 如果需要获取医生的详细用户信息，可以查询Users表
                try
                {
                    mysqlx::Table users_table = dbManager->getSchema()->getTable("Users");
                    mysqlx::RowResult user_result = users_table.select("name", "phone", "email")
                                                        .where("id = :id")
                                                        .bind("id", row[0].get<int>())
                                                        .execute();
                    auto user_row = user_result.fetchOne();
                    if (user_row)
                    {
                        doctor["user_name"] = user_row[0].get<std::string>();
                        doctor["phone"] = user_row[1].get<std::string>();
                        doctor["email"] = user_row[2].get<std::string>();
                    }
                }
                catch (...)
                {
                    // 如果获取用户信息失败，继续处理
                }

                doctor_list.push_back(doctor);
            }

            // 返回成功响应
            nlohmann::json response;
            response["success"] = true;
            response["data"] = doctor_list;
            return ResponseHelper::success(req, response);
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "Database error: " << e.what() << std::endl;
            nlohmann::json error_response;
            error_response["success"] = false;
            error_response["error"] = "Database operation failed";
            return ResponseHelper::system_error(req);
        }
    }
    catch (const std::exception &e)
    {
        nlohmann::json error_response;
        error_response["success"] = false;
        error_response["error"] = "Failed to fetch doctor list";
        return ResponseHelper::system_error(req);
    }
}

// 取消预约接口
crow::response ReservationHandler::cancelReservation(const crow::request &req, int user_id, int reservation_id)
{
    try
    {
        nlohmann::json request_body;
        crow::response res;

        if (!parseJsonBody(req, res, request_body))
        {
            return ResponseHelper::custom(req, 400, "error: Invalid JSON");
        }
        // 检查数据库连接是否存在
        if (!dbManager || !dbManager->getSchema() || !dbManager->getSession())
        {
            return ResponseHelper::custom(req, 500, "error: Database connection not available");
        }

        if (request_body.find("email") == request_body.end() ||
            request_body.find("user_id") == request_body.end() ||
            request_body.find("id") == request_body.end())
        {
            return ResponseHelper::custom(req, 400, "error: Missing required fields");
        }

        // 检查request_body["email"]是否为字符串类型
        // 如果是字符串，直接获取该字符串值
        // 如果不是字符串，将其转为字符串格式（dump方法）
        std::string email = request_body["email"].is_string() ? request_body["email"].get<std::string>() : request_body["email"].dump();
        int user_id = request_body["user_id"].is_number() ? request_body["user_id"].get<int>() : std::stoi(request_body["user_id"].dump());
        int reaservation_id = request_body["id"].is_number() ? request_body["id"].get<int>() : std::stoi(request_body["id"].dump());

        std::string status = "已取消";

        // 获取预约表
        mysqlx::Table reservation_table = dbManager->getSchema()->getTable("reaservations");

        // 验证用户和预约记录是否匹配
        mysqlx::RowResult reservation_result = reservation_table.select("user_id")
                                                   .where("id = :id")
                                                   .bind("id", reaservation_id)
                                                   .execute();

        auto reservation_row = reservation_result.fetchOne();
        if (!reservation_row)
        {
            return ResponseHelper::custom(req, 400, "error: Reservation record does not exist");
        }

        nlohmann::json response;
        if (reservation_row[0].get<int>() == user_id) // 操作用户和预约记录用户匹配
        {

            // 更新数据库
            mysqlx::TableUpdate update_op = reservation_table.update();
            mysqlx::Result result = update_op.set("status", status)
                                        .where("id = :id")
                                        .bind("id", reaservation_id)
                                        .execute();

            // 检查是否有记录被更新
            if (result.getAffectedItemsCount() > 0)
            {
                // 返回成功响应
                response["success"] = true;
                response["message"] = "取消成功";
                return ResponseHelper::success(req, response);
            }
            else
            {
                // 没有找到对应的记录
                response["success"] = false;
                response["error"] = "未找到指定的预约记录";
                return ResponseHelper::custom(req, 404, response);
            }
        }
        else // 操作用户和预约记录用户不匹配
        {
            // 预约记录不匹配
            response["success"] = false;
            response["error"] = "预约记录不匹配";
            return ResponseHelper::custom(req, 400, response);
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::custom(req, 500, "error: Failed to cancel reservation, details: " + std::string(e.what()) + "\"}");
    }
}

//  删除预约记录接口
crow::response ReservationHandler::deleteReservation(const crow::request &req, int user_id, int reservation_id)
{
    try
    {
        // 检查数据库连接是否存在
        if (!dbManager || !dbManager->getSchema() || !dbManager->getSession())
        {
            return ResponseHelper::custom(req, 500, "error: Database connection not available");
        }

        // 获取预约表
        mysqlx::Table reservation_table = dbManager->getSchema()->getTable("reaservations");

        mysqlx::RowResult reservation_result = reservation_table.select("user_id")
                                           .where("id = :id")
                                           .bind("id", reservation_id)
                                           .execute();

        auto reservation_row = reservation_result.fetchOne();

        if(!reservation_row)
        {
            return ResponseHelper::custom(req, 400, "error: Reservation record does not exist");
        }

        if (reservation_row[0].get<int>() == user_id)       // 操作用户和预约记录用户匹配才允许删除
        {

            // 删除指定ID的预约记录
            mysqlx::Result result = reservation_table.remove()
                                        .where("id = :id")
                                        .bind("id", reservation_id)
                                        .execute();

            // 检查是否有记录被删除
            if (result.getAffectedItemsCount() > 0)
            {
                // 返回成功响应
                nlohmann::json response;
                response["success"] = true;
                response["message"] = "预约记录删除成功";
                return ResponseHelper::success(req, response);
            }
            else
            {
                // 没有找到对应的记录
                nlohmann::json response;
                response["success"] = false;
                response["error"] = "未找到指定的预约记录";
                return ResponseHelper::custom(req, 404, response);
            }
        }
        else
        {
            // 预约记录不匹配
            nlohmann::json response;
            response["success"] = false;
            response["error"] = "预约记录不匹配";
            return ResponseHelper::custom(req, 400, response);
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::custom(req, 500, "error: Failed to delete reservation, details: " + std::string(e.what()) + "\"}");
    }
}
