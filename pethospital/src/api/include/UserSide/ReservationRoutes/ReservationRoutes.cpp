#include "ReservationRoutes.h"

void ReservationRoutes::setupReservationRoutes(crow::SimpleApp &app, DatabaseManagerInterface *dbManager)
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
        return;

    // 创建 ReservationHandler 实例
    ReservationHandler handler(dbManager);

    // 存储预约记录.
    CROW_ROUTE(app, "/api/reservate/record")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                    { 
            try {
                // 处理OPTIONS预检请求 - 这是为了CORS跨域支持
                initializeOPTIONS(req,res);

                // 解析请求体中的 JSON 数据
                nlohmann::json request_body;

                // parseJsonBody 函数用于解析请求体中的 JSON 数据
                if (!parseJsonBody(req, res, request_body))
                {
                    res = ResponseHelper::custom(req, 400, "error: Invalid JSON");
                    res.end();
                    return;
                }


                std::string name = request_body["name"].is_string() ? request_body["name"].get<std::string>() : request_body["name"].dump();
                std::string email = request_body["email"].is_string() ? request_body["email"].get<std::string>() : request_body["email"].dump();
                std::string phone = request_body["phone"].is_string() ? request_body["phone"].get<std::string>() : request_body["phone"].dump();

                // 从数据库中获取用户信息
                int user_id = 0;
                int doctor_id = 0;
                std::string date = "";
                std::string time_slot = "";
                std::string status = "预约成功";
                std::string creation_time = "";

                // 安全获取预约信息字段
                if (request_body.find("doctor_id") != request_body.end() && !request_body["doctor_id"].is_null())
                {
                    doctor_id = request_body["doctor_id"].is_number() ? request_body["doctor_id"].get<int>() : std::stoi(request_body["doctor_id"].dump());
                }
                if (request_body.find("date") != request_body.end() && !request_body["date"].is_null())
                {
                    date = request_body["date"].is_string() ? request_body["date"].get<std::string>() : request_body["date"].dump();
                }
                if (request_body.find("slot") != request_body.end() && !request_body["slot"].is_null())
                {
                    time_slot = request_body["slot"].is_string() ? request_body["slot"].get<std::string>() : request_body["slot"].dump();
                }
                // 获得预约记录创建时间
                creation_time = getCreateTime();
            
                // 直接调用处理器方法
                crow::response handlerResponse = handler.createReservation(req, user_id, name, email, phone, doctor_id, date, time_slot, status, creation_time);

                ProcessHandlerResponse(req, res, handlerResponse);
            
            } catch (const std::exception& e) {
                res = ResponseHelper::custom(req, 500, "error: Failed to save reservation");
            }
            res.end(); });

    // 获取预约记录列表路由.
    CROW_ROUTE(app, "/api/reservate/getrecord")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                   {
            try {
                // 处理OPTIONS预检请求
                initializeOPTIONS(req, res);

                // 用户 ID 是身份认证信息，通常由前端从 JWT token 或 session 中获取，并放在请求头中更安全地传递

                //  身份相关的数据（如 user_id、token 等）→ 通过请求头获取
                //  业务相关的数据（如预约日期、医生ID等）→ 通过请求体 JSON 获取
                std::string user_id_str = req.get_header_value("user_id");
                //  Crow 框架的 req.get_header_value() 方法返回的是一个字符串类型的值,需要通过 std::stoi 转换为整数类型

                if (user_id_str.empty()) {
                    res = ResponseHelper::custom(req, 400, "Missing user_id in headers");
                    res.end();
                    return;
                }

                int user_id = std::stoi(user_id_str);

                crow::response handlerResponse = handler.getReservations(req, user_id);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "error: Failed to fetch reservations, details: " + std::string(e.what()) + "\"");
            }
            res.end(); });

    //  预约提前数据路由.
    CROW_ROUTE(app, "/api/reservate/getData")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                   {
            try {
                
                nlohmann::json schedule = handler.getReservationData();

                res = ResponseHelper::success(req, schedule);
            } catch (const std::exception& e) {
                res = ResponseHelper::custom(req, 500, "error: Failed to generate schedule");
            }
            res.end(); });

    // 获取医生列表路由.
    CROW_ROUTE(app, "/api/reservate/getDoctor")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                   {
            try {
                // 生成医生列表
                crow::response handlerResponse = handler.getDoctorList(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::custom(req, 500, "error, Failed to get doctor list");
            }
            res.end(); });

    // 更新预约记录路由
    CROW_ROUTE(app, "/api/reservate/update")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                    {
            try {
                // 处理OPTIONS预检请求
                initializeOPTIONS(req, res);

                nlohmann::json request_body;
                if (!parseJsonBody(req, res, request_body)) {
                    res = ResponseHelper::custom(req, 400, "error: Invalid JSON");
                    res.end();
                    return;
                }

                std::string reservation_id_str = request_body["reservation_id"].is_string() ? request_body["reservation_id"].get<std::string>() : request_body["reservation_id"].dump();
                if (reservation_id_str.empty()) {
                    res = ResponseHelper::custom(req, 400, "Missing reservation_id in headers");
                    res.end();
                    return;
                }
                
                int reservation_id = std::stoi(reservation_id_str);

                crow::response handlerResponse = handler.updateReservation(req, reservation_id);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                res = ResponseHelper::custom(req, 500, "error: Failed to update reservation, details: " + std::string(e.what()) + "\"}");
            }
            res.end(); });

    // 取消预约记录路由.
    CROW_ROUTE(app, "/api/reservate/cancel")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                    {
            try {
                // 处理OPTIONS预检请求
                initializeOPTIONS(req,res);

                nlohmann::json request_body;
                if (!parseJsonBody(req, res, request_body)) {
                    res = ResponseHelper::custom(req, 400, "error: Invalid JSON");
                    res.end();
                    return;
                }

                std::string user_id_str = req.get_header_value("user_id");
                std::string reservation_id_str = request_body["reservation_id"].is_string() ? request_body["reservation_id"].get<std::string>() : request_body["reservation_id"].dump();
                if (user_id_str.empty() || reservation_id_str.empty()) {
                    res = ResponseHelper::custom(req, 400, "Missing user_id or reservation_id in headers");
                    res.end();
                    return;
                }

                int user_id = std::stoi(user_id_str);
                int reservation_id = std::stoi(reservation_id_str);

                crow::response handlerResponse = handler.cancelReservation(req, user_id, reservation_id);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                res = ResponseHelper::custom(req, 500, "error: Failed to cancel reservation");
            } });

    // 删除预约记录路由.
    CROW_ROUTE(app, "/api/reservate/deleterecord")
        .methods(crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                      {
            try {
                // 处理OPTIONS预检请求
                initializeOPTIONS(req, res);

                nlohmann::json request_body;
                if (!parseJsonBody(req, res, request_body)) {
                    res = ResponseHelper::custom(req, 400, "error: Invalid JSON");
                    res.end();
                    return;
                }

                std::string user_id_str = req.get_header_value("user_id");
                std::string reaservation_id_str = request_body["reaservation_id"].is_string() ? request_body["reaservation_id"].get<std::string>() : request_body["reaservation_id"].dump();
                if (reaservation_id_str.empty())
                {
                    res = ResponseHelper::custom(req, 400, "Missing reaservation_id in headers");
                    res.end();
                    return;
                }

                int user_id = std::stoi(user_id_str);
                int reaservation_id = std::stoi(reaservation_id_str);

                crow::response handlerResponse = handler.deleteReservation(req, user_id, reaservation_id);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                res = ResponseHelper::custom(req, 500, "error: Failed to delete reservation, details: " + std::string(e.what()) + "\"");
            }
            res.end(); });

    routes_setup = true;
}