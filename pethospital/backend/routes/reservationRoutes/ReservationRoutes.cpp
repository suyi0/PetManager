#include "ReservationRoutes.h"
#include "../../controllers/OperationLogger/OperationLogger.h"

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
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "预约", "创建预约");
                    return;
                }

                ReservationHandler handler(dbManager);
                
                // 解析请求体中的 JSON 数据
                auto jsonOpt = handler.parseJson(req, res);
                if (!jsonOpt)
                {
                    OperationLogger::FinishLoggedRoute(dbManager, req, res, "预约", "创建预约", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    return;
                }
                nlohmann::json& request_body = jsonOpt.value();

                // 从数据库中获取用户信息
                int doctor_id = 0;
                int pet_id = 0;
                std::string date = "";
                std::string time_slot = "";
                std::string status = "预约成功";

                auto readIntField = [&request_body](const std::string &snakeKey, const std::string &camelKey) -> int
                {
                    const nlohmann::json *value = nullptr;
                    if (request_body.find(snakeKey) != request_body.end() && !request_body[snakeKey].is_null())
                    {
                        value = &request_body[snakeKey];
                    }
                    else if (request_body.find(camelKey) != request_body.end() && !request_body[camelKey].is_null())
                    {
                        value = &request_body[camelKey];
                    }

                    if (!value)
                    {
                        return 0;
                    }
                    if (value->is_number_integer())
                    {
                        return value->get<int>();
                    }
                    if (value->is_string())
                    {
                        return std::stoi(value->get<std::string>());
                    }
                    return 0;
                };

                // 安全获取预约信息字段，兼容前端 doctorId 和后端 doctor_id 两种命名。
                doctor_id = readIntField("doctor_id", "doctorId");
                pet_id = readIntField("pet_id", "petId");
                if (request_body.find("date") != request_body.end() && !request_body["date"].is_null())
                {
                    date = request_body["date"].is_string() ? request_body["date"].get<std::string>() : request_body["date"].dump();
                }
                if (request_body.find("slot") != request_body.end() && !request_body["slot"].is_null())
                {
                    time_slot = request_body["slot"].is_string() ? request_body["slot"].get<std::string>() : request_body["slot"].dump();
                }
            
                // 直接调用处理器方法
                crow::response handlerResponse = handler.createReservation(req, userId, pet_id, doctor_id, date, time_slot, status);

                ProcessHandlerResponse(req, res, handlerResponse);
            
            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "预约", "创建预约", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::operation_failed(req, "Failed to save reservation", e.what());
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "预约", "创建预约", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 获取预约记录列表路由.
    CROW_ROUTE(app, "/api/reservate/getrecord")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "预约", "获取预约记录");
                    return;
                }
                
                ReservationHandler handler(dbManager);
                crow::response handlerResponse = handler.getReservations(req, userId);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取预约记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::operation_failed(req, "Failed to fetch reservations", e.what());
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "预约", "获取预约记录", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    //  获取预约日期路由.
    CROW_ROUTE(app, "/api/reservate/getData")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            try {
                
                ReservationHandler handler(dbManager);
                nlohmann::json schedule = handler.getReservationData();

                res = ResponseHelper::success(req, schedule);
            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取预约日期", e.what());
                res = ResponseHelper::operation_failed(req, "Failed to generate schedule", e.what());
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "预约", "获取预约日期", std::nullopt, false); });

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
                OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取医生列表", e.what());
                res = ResponseHelper::operation_failed(req, "Failed to get doctor list", e.what());
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "预约", "获取医生列表", std::nullopt, false); });

    // 更新预约记录路由
    CROW_ROUTE(app, "/api/reservate/update/<int>")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int reservation_id)
                                                                    {
            int userId = -1;
            try {
                    userId = isValidUserToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "预约", "更新预约");
                        return;
                    }

                    ReservationHandler handler(dbManager);
                    crow::response handlerResponse = handler.updateReservation(req, userId, reservation_id);

                    ProcessHandlerResponse(req, res, handlerResponse);

                } catch (const std::exception& e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "预约", "更新预约", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::operation_failed(req, "Failed to update reservation", e.what());
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "预约", "更新预约", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 取消预约记录路由.
    CROW_ROUTE(app, "/api/reservate/cancel/<int>")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int reservation_id)
                                                                    {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "预约", "取消预约");
                    return;
                }

                ReservationHandler handler(dbManager);
                crow::response handlerResponse = handler.cancelReservation(req, userId, reservation_id);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "预约", "取消预约", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::operation_failed(req, "Failed to cancel reservation", e.what());
            } 
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "预约", "取消预约", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 删除预约记录路由.
    CROW_ROUTE(app, "/api/reservate/deleterecord/<int>")
        .methods(crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int reservation_id)
                                                                      {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "预约", "删除预约记录");
                    return;
                }

                ReservationHandler handler(dbManager);
                crow::response handlerResponse = handler.deleteReservation(req, userId, reservation_id);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "预约", "删除预约记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::operation_failed(req, "Failed to delete reservation", e.what());
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "预约", "删除预约记录", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    routes_setup = true;
}
