#include "ReservationRoutes.h"

void ReservationRoutes::setupReservationRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
        return;

    // 存储预约记录.
    CROW_ROUTE(app, "/api/reservate/record")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    { 
            try
            {
                int userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    res.end();
                    return;
                }

                ReservationHandler handler(dbManager);
                
                // 解析请求体中的 JSON 数据
                auto jsonOpt = handler.parseJson(req, res);
                if (!jsonOpt)
                {
                    res.end();
                    return;
                }
                nlohmann::json& request_body = jsonOpt.value();

                std::string name = request_body["name"].is_string() ? request_body["name"].get<std::string>() : request_body["name"].dump();
                std::string email = request_body["email"].is_string() ? request_body["email"].get<std::string>() : request_body["email"].dump();
                std::string phone = request_body["phone"].is_string() ? request_body["phone"].get<std::string>() : request_body["phone"].dump();

                // 从请求头中获取用户 ID
                std::string token = req.get_header_value("Authorization").substr(7);
                int user_id = JwtUtils::getUserIdFromToken(token);

                // 从数据库中获取用户信息
                int doctor_id = 0;
                std::string date = "";
                std::string time_slot = "";
                std::string status = "预约成功";

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
            
                // 直接调用处理器方法
                crow::response handlerResponse = handler.createReservation(req, user_id, name, email, phone, doctor_id, date, time_slot, status);

                ProcessHandlerResponse(req, res, handlerResponse);
            
            } catch (const std::exception& e) {
                res = ResponseHelper::operation_failed(req, "Failed to save reservation", e.what());
            }
            res.end(); });

    // 获取预约记录列表路由.
    CROW_ROUTE(app, "/api/reservate/getrecord")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            try
            {

                int userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    res.end();
                    return;
                }
                
                ReservationHandler handler(dbManager);
                crow::response handlerResponse = handler.getReservations(req, userId);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                res = ResponseHelper::operation_failed(req, "Failed to fetch reservations", e.what());
            }
            res.end(); });

    //  预约提前数据路由.
    CROW_ROUTE(app, "/api/reservate/getData")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            try {
                
                ReservationHandler handler(dbManager);
                nlohmann::json schedule = handler.getReservationData();

                res = ResponseHelper::success(req, schedule);
            } catch (const std::exception& e) {
                res = ResponseHelper::operation_failed(req, "Failed to generate schedule", e.what());
            }
            res.end(); });

    // 获取医生列表路由.
    CROW_ROUTE(app, "/api/reservate/getDoctor")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            try {
                ReservationHandler handler(dbManager);
                // 生成医生列表
                crow::response handlerResponse = handler.getDoctorList(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::operation_failed(req, "Failed to get doctor list", e.what());
            }
            res.end(); });

    // 更新预约记录路由
    CROW_ROUTE(app, "/api/reservate/update/<int>")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int reservation_id)
                                                                    {
            try {
                    int userId = isValidUserToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        res.end();
                        return;
                    }

                    ReservationHandler handler(dbManager);
                    crow::response handlerResponse = handler.updateReservation(req, reservation_id);

                    ProcessHandlerResponse(req, res, handlerResponse);

                } catch (const std::exception& e) {
                    res = ResponseHelper::operation_failed(req, "Failed to update reservation", e.what());
                }
                res.end(); });

    // 取消预约记录路由.
    CROW_ROUTE(app, "/api/reservate/cancel/<int>")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int reservation_id)
                                                                    {
            try
            {
                int userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    res.end();
                    return;
                }

                ReservationHandler handler(dbManager);
                crow::response handlerResponse = handler.cancelReservation(req, userId, reservation_id);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                res = ResponseHelper::operation_failed(req, "Failed to cancel reservation", e.what());
            } 
            res.end(); });

    // 删除预约记录路由.
    CROW_ROUTE(app, "/api/reservate/deleterecord/<int>")
        .methods(crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int reservation_id)
                                                                      {
            try
            {
                int userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    res.end();
                    return;
                }

                ReservationHandler handler(dbManager);
                crow::response handlerResponse = handler.deleteReservation(req, userId, reservation_id);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                res = ResponseHelper::operation_failed(req, "Failed to delete reservation", e.what());
            }
            res.end(); });

    routes_setup = true;
}
