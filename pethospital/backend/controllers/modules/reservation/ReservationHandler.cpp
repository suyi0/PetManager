#include "ReservationHandler.h"
#include "RoleTypeUtils/RoleTypeUtils.h"
#include "../../OperationLogger/OperationLogger.h"
#include <iostream>

namespace
{
std::string getTodayDate()
{
    const boost::posix_time::ptime currentDateTime = boost::posix_time::second_clock::local_time();
    return formatDateOnly(currentDateTime);
}

nlohmann::json buildDoctorJson(const mysqlx::Row &row)
{
    nlohmann::json doctor;
    doctor["doctor_id"] = row[0].isNull() ? 0 : row[0].get<int>();
    doctor["id"] = doctor["doctor_id"];
    doctor["name"] = row[1].isNull() ? "" : row[1].get<std::string>();
    doctor["phone"] = row[2].isNull() ? "" : row[2].get<std::string>();
    doctor["email"] = row[3].isNull() ? "" : row[3].get<std::string>();
    doctor["specialty"] = row[4].isNull() ? "" : row[4].get<std::string>();
    doctor["status"] = row[5].isNull() ? "offline" : row[5].get<std::string>();
    return doctor;
}
}

// 创建预约表记录接口
crow::response ReservationHandler::createReservation(const crow::request &req, int user_id, std::string name, std::string email, std::string phone, int doctor_id, std::string date, std::string time_slot, std::string status)
{
    try
    {
        // 检查数据库连接是否存在
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "创建预约", "database connection failed", user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        // 检查必要字段是否存在

        if (user_id != 0 && doctor_id != 0 && !date.empty() && !time_slot.empty())
        {
            try
            {
                dbManager->getSession()
                    ->sql("INSERT INTO reaservations (user_id, doctor_id, date, time_slot, status) VALUES (?, ?, ?, ?, ?)")
                    .bind(user_id, doctor_id, date, time_slot, status)
                    .execute();
            }
            catch (const mysqlx::Error &e)
            {
                std::cerr << "Database error: " << e.what() << std::endl;
                OperationLogger::LogExceptionOperation(dbManager, req, "预约", "创建预约", e.what(), user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
                return ResponseHelper::database_error(req, "Failed to create reservation", e.what());
            }

            // 返回成功响应
            nlohmann::json response;
            response["reservation_status"] = status;
            response["message"] = "预约成功";
            return ResponseHelper::success(req, response);
        }
        else
        {
            return ResponseHelper::validation(req, "Missing required fields");
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "创建预约", e.what(), user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to save reservation", e.what());
    }
}

//  获得预约记录接口
crow::response ReservationHandler::getReservations(const crow::request &req, int user_id)
{
    try
    {
        // 检查数据库连接是否存在
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取预约记录", "database connection failed", user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        try
        {
            mysqlx::SqlResult result = dbManager->getSession()
                                           ->sql("SELECT id, user_id, doctor_id, date, time_slot, status, created_at "
                                                 "FROM reaservations WHERE user_id = ? ORDER BY date DESC")
                                           .bind(user_id)
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
                record["created_at"] = row[6].get<std::string>();
                response_data.push_back(record);
            }

            // 返回成功响应
            return ResponseHelper::success(req, response_data);
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "Database error: " << e.what() << std::endl;
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取预约记录", e.what(), user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
            return ResponseHelper::database_error(req, "Failed to fetch reservations", e.what());
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取预约记录", e.what(), user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to fetch reservations", e.what());
    }
}

// 更新预约记录接口
crow::response ReservationHandler::updateReservation(const crow::request &req, int id)
{
    try
    {
        // 解析请求体中的 JSON 数据
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        bool has_changes = false;
        if (request_body.find("date") != request_body.end())
        {
            dbManager->getSession()->sql("UPDATE reaservations SET date = ? WHERE id = ?")
                .bind(request_body["date"].get<std::string>(), id)
                .execute();
            has_changes = true;
        }
        if (request_body.find("time_slot") != request_body.end())
        {
            dbManager->getSession()->sql("UPDATE reaservations SET time_slot = ? WHERE id = ?")
                .bind(request_body["time_slot"].get<std::string>(), id)
                .execute();
            has_changes = true;
        }
        if (request_body.find("status") != request_body.end())
        {
            dbManager->getSession()->sql("UPDATE reaservations SET status = ? WHERE id = ?")
                .bind(request_body["status"].get<std::string>(), id)
                .execute();
            has_changes = true;
        }

        // 检查是否有记录被更新
        if (has_changes)
        {
            // 返回成功响应
            nlohmann::json response;
            response["message"] = "预约记录更新成功";
            response["reservation_id"] = id;
            return ResponseHelper::success(req, response);
        }
        else
        {
            return ResponseHelper::notFound(req, "未找到指定的预约记录");
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "更新预约", e.what());
        return ResponseHelper::operation_failed(req, "Failed to update reservation", e.what());
    }
}

// 获取预约时间表数据接口
nlohmann::json ReservationHandler::getReservationData()
{
    // 这里应该生成并返回时间表
    Reservate r;
    auto schedule = r.generateSchedule();
    return schedule;
}

// 获取医生列表接口
crow::response ReservationHandler::getDoctorList(const crow::request &req)
{
    if (!checkDbConnection())
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取医生列表", "database connection failed");
        return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
    }

    try
    {
        const int doctorRoleId = RoleTypeUtils::getRoleId(dbManager, "医生");
        if (doctorRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "医生角色不存在");
        }

        const std::string todayDate = getTodayDate();

        mysqlx::RowResult result = dbManager->getSession()
                                       ->sql("SELECT u.id, u.name, u.phone, u.email, u.user_specialty, "
                                             "COALESCE(od.status, 'offline') "
                                             "FROM users AS u "
                                             "LEFT JOIN onlineDoctors AS od "
                                             "ON od.doctor_id = u.id AND od.date = ? "
                                             "WHERE u.type_id = ?")
                                      .bind(todayDate, doctorRoleId)
                                      .execute();

        nlohmann::json doctorList = nlohmann::json::array();
        for (const auto &row : result)
        {
            doctorList.push_back(buildDoctorJson(row));
        }

        return ResponseHelper::success(req, doctorList);
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "Database error: " << e.what() << std::endl;
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取医生列表", e.what());
        return ResponseHelper::database_error(req, "Failed to fetch doctor list", e.what());
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取医生列表", e.what());
        return ResponseHelper::operation_failed(req, "Failed to fetch doctor list", e.what());
    }
}

// 取消预约接口
crow::response ReservationHandler::cancelReservation(const crow::request &req, int user_id, int reservation_id)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        if (request_body.find("email") == request_body.end() ||
            request_body.find("user_id") == request_body.end() ||
            request_body.find("id") == request_body.end())
        {
            return ResponseHelper::validation(req, "Missing required fields");
        }

        // 检查request_body["email"]是否为字符串类型
        // 如果是字符串，直接获取该字符串值
        // 如果不是字符串，将其转为字符串格式（dump方法）
        std::string email = request_body["email"].is_string() ? request_body["email"].get<std::string>() : request_body["email"].dump();
        int request_user_id = request_body["user_id"].is_number() ? request_body["user_id"].get<int>() : std::stoi(request_body["user_id"].dump());
        int request_reservation_id = request_body["id"].is_number() ? request_body["id"].get<int>() : std::stoi(request_body["id"].dump());

        (void)email;
        (void)reservation_id;

        std::string status = "已取消";

        // 验证用户和预约记录是否匹配
        mysqlx::SqlResult reservation_result = dbManager->getSession()
                                                   ->sql("SELECT user_id FROM reaservations WHERE id = ?")
                                                   .bind(request_reservation_id)
                                                   .execute();

        auto reservation_row = reservation_result.fetchOne();
        if (!reservation_row)
        {
            return ResponseHelper::notFound(req, "Reservation record does not exist");
        }

        if (reservation_row[0].get<int>() == request_user_id) // 操作用户和预约记录用户匹配
        {

            mysqlx::SqlResult result = dbManager->getSession()
                                        ->sql("UPDATE reaservations SET status = ? WHERE id = ?")
                                        .bind(status, request_reservation_id)
                                        .execute();

            // 检查是否有记录被更新
            if (result.getAffectedItemsCount() > 0)
            {
                // 返回成功响应
                nlohmann::json response;
                response["message"] = "取消成功";
                response["reservation_id"] = request_reservation_id;
                response["status"] = status;
                return ResponseHelper::success(req, response);
            }
            else
            {
                return ResponseHelper::notFound(req, "未找到指定的预约记录");
            }
        }
        else // 操作用户和预约记录用户不匹配
        {
            return ResponseHelper::permission_denied(
                req,
                "预约记录不匹配",
                "Reservation record does not belong to current user");
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "取消预约", e.what(), user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to cancel reservation", e.what());
    }
}

//  删除预约记录接口
crow::response ReservationHandler::deleteReservation(const crow::request &req, int user_id, int reservation_id)
{
    try
    {
        // 检查数据库连接是否存在
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "删除预约记录", "database connection failed", user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        mysqlx::SqlResult reservation_result = dbManager->getSession()
                                                   ->sql("SELECT user_id FROM reaservations WHERE id = ?")
                                                   .bind(reservation_id)
                                                   .execute();

        auto reservation_row = reservation_result.fetchOne();

        if (!reservation_row)
        {
            return ResponseHelper::notFound(req, "Reservation record does not exist");
        }

        if (reservation_row[0].get<int>() == user_id) // 操作用户和预约记录用户匹配才允许删除
        {

            // 删除指定ID的预约记录
            mysqlx::SqlResult result = dbManager->getSession()
                                        ->sql("DELETE FROM reaservations WHERE id = ?")
                                        .bind(reservation_id)
                                        .execute();

            // 检查是否有记录被删除
            if (result.getAffectedItemsCount() > 0)
            {
                // 返回成功响应
                nlohmann::json response;
                response["message"] = "预约记录删除成功";
                response["reservation_id"] = reservation_id;
                return ResponseHelper::success(req, response);
            }
            else
            {
                return ResponseHelper::notFound(req, "未找到指定的预约记录");
            }
        }
        else
        {
            return ResponseHelper::permission_denied(
                req,
                "预约记录不匹配",
                "Reservation record does not belong to current user");
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "删除预约记录", e.what(), user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to delete reservation", e.what());
    }
}
