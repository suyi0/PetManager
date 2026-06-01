#include "UserRoutes.h"
#include "../../controllers/OperationLogger/OperationLogger.h"

std::unordered_map<std::string, std::chrono::steady_clock::time_point> email_check_last_access;

void UserRoutes::setupUserRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
        return;

    
    // 添加注册路由
    CROW_ROUTE(app, "/api/user/register")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try
            {
                userHandler handler(dbManager);
                crow::response handlerResponse = handler.userUpdate(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            }
            catch (const std::exception &e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "注册账号", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "注册账号", std::nullopt, false); });
    
    // 添加登录路由
    CROW_ROUTE(app, "/api/user/login")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try {
                userHandler handler(dbManager);
                crow::response handlerResponse = handler.userLogin(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch(const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "账号登录", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
                                                        
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "账号登录"); });

    // 添加用户资料更新路由
    CROW_ROUTE(app, "/api/user/profile")
        .methods(crow::HTTPMethod::Put, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", "更新资料");
                    return;
                }
                userHandler handler(dbManager);
                crow::response handlerResponse = handler.userUpdate(req, userId);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "更新资料", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }

            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "更新资料", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 上传头像
    // 文件上传请求,客户端发送的是 multipart/form-data 格式，不是 JSON 格式
    CROW_ROUTE(app, "/api/user/avatar")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", "上传头像");
                    return;
                }
                userHandler handler(dbManager);
                crow::response handlerResponse = handler.userUploadAvatar(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "上传头像", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "上传头像", userId > 0 ? std::optional<int>(userId) : std::nullopt); });
        
    // 添加静态文件服务路由
    CROW_ROUTE(app, "/uploads/<string>")
        .methods(crow::HTTPMethod::Get)([dbManager](const crow::request &req, crow::response &res, std::string filename)
                                        {
            try
            {
                userHandler handler(dbManager);
                crow::response handlerResponse = handler.upload(req, filename);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception &e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "获取上传文件", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "获取上传文件", std::nullopt, false); });

    // 添加宠物档案管理路由
    CROW_ROUTE(app, "/api/user/petProfiles")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                                           {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);
                if (res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", "管理宠物档案");
                    return;
                }

                petCommonHandler handler(dbManager);
                crow::response handlerResponse =
                    req.method == crow::HTTPMethod::Post
                        ? handler.createPetProfile(req, userId)
                        : handler.getPetProfiles(req, userId);

                ProcessHandlerResponse(req, res, handlerResponse);
            }
            catch (const std::exception &e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "管理宠物档案", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "管理宠物档案", userId > 0 ? std::optional<int>(userId) : std::nullopt, req.method != crow::HTTPMethod::Get); });

    // 更新/删除宠物档案管理路由
    CROW_ROUTE(app, "/api/user/petProfiles/<int>")
        .methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int petId)
                                                                                             {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);
                if (res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", "管理宠物档案");
                    return;
                }

                petCommonHandler handler(dbManager);
                crow::response handlerResponse =
                    req.method == crow::HTTPMethod::Delete
                        ? handler.deletePetProfile(req, userId, petId)
                        : handler.updatePetProfile(req, userId, petId);

                ProcessHandlerResponse(req, res, handlerResponse);
            }
            catch (const std::exception &e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "管理宠物档案", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "管理宠物档案", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 用户预约摘要与创建预约路由.
    CROW_ROUTE(app, "/api/user/reservations")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                                           {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if (res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", req.method == crow::HTTPMethod::Post ? "创建预约" : "获取预约摘要");
                    return;
                }

                if (req.method == crow::HTTPMethod::Get)
                {
                    reservationCommonHandler handler(dbManager);
                    crow::response handlerResponse = handler.getReservationSummary(req, userId);

                    ProcessHandlerResponse(req, res, handlerResponse);
                    OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "获取预约摘要", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
                    return;
                }

                userHandler handler(dbManager);

                // 解析请求体中的 JSON 数据
                auto jsonOpt = handler.parseJson(req, res);
                if (!jsonOpt)
                {
                    OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "创建预约", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    return;
                }
                nlohmann::json& request_body = jsonOpt.value();

                // 从数据库中获取用户信息
                int doctor_id = 0;
                int pet_id = 0;
                std::string date = "";
                std::string time_slot = "";
                std::string reservation_type = "";
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
                else if (request_body.find("time_slot") != request_body.end() && !request_body["time_slot"].is_null())
                {
                    time_slot = request_body["time_slot"].is_string() ? request_body["time_slot"].get<std::string>() : request_body["time_slot"].dump();
                }
                if (request_body.find("reservation_type") != request_body.end() && !request_body["reservation_type"].is_null())
                {
                    reservation_type = request_body["reservation_type"].is_string() ? request_body["reservation_type"].get<std::string>() : request_body["reservation_type"].dump();
                }
                else if (request_body.find("reservationType") != request_body.end() && !request_body["reservationType"].is_null())
                {
                    reservation_type = request_body["reservationType"].is_string() ? request_body["reservationType"].get<std::string>() : request_body["reservationType"].dump();
                }
            
                // 直接调用处理器方法
                crow::response handlerResponse = handler.createReservation(req, userId, pet_id, doctor_id, reservation_type, date, time_slot, status);

                ProcessHandlerResponse(req, res, handlerResponse);
            
            }
            catch (const std::exception &e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", req.method == crow::HTTPMethod::Post ? "创建预约" : "获取预约摘要", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::operation_failed(req, req.method == crow::HTTPMethod::Post ? "Failed to save reservation" : "Failed to fetch reservation summary", e.what());
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "创建预约", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 用户预约详情与删除预约记录路由.
    CROW_ROUTE(app, "/api/user/reservations/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int reservationId)
                                                                                             {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", req.method == crow::HTTPMethod::Delete ? "删除预约记录" : "获取预约记录");
                    return;
                }

                crow::response handlerResponse;
                if (req.method == crow::HTTPMethod::Delete)
                {
                    userHandler handler(dbManager);
                    handlerResponse = handler.deleteReservation(req, userId, reservationId);
                }
                else
                {
                    reservationCommonHandler handler(dbManager);
                    handlerResponse = handler.getReservationInformation(req, reservationId);
                }

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", req.method == crow::HTTPMethod::Delete ? "删除预约记录" : "获取预约记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::operation_failed(req, req.method == crow::HTTPMethod::Delete ? "Failed to delete reservation" : "Failed to fetch reservations", e.what());
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", req.method == crow::HTTPMethod::Delete ? "删除预约记录" : "获取预约记录", userId > 0 ? std::optional<int>(userId) : std::nullopt, req.method == crow::HTTPMethod::Delete); });

    //  获取预约日期路由.
    CROW_ROUTE(app, "/api/user/reservations/dates")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            try {
                userHandler handler(dbManager);
                nlohmann::json schedule = handler.getReservationDate();

                res = ResponseHelper::success(req, schedule);
            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "获取预约日期", e.what());
                res = ResponseHelper::operation_failed(req, "Failed to generate schedule", e.what());
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "获取预约日期", std::nullopt, false); });

    // 获取可预约的医生列表路由.
    CROW_ROUTE(app, "/api/user/reservations/doctors")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            try {
                userHandler handler(dbManager);

                // 生成医生列表
                crow::response handlerResponse = handler.getDoctorList(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "获取医生列表", e.what());
                res = ResponseHelper::operation_failed(req, "Failed to get doctor list", e.what());
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "获取医生列表", std::nullopt, false); });

    // 取消预约记录路由.
    CROW_ROUTE(app, "/api/user/reservations/<int>/cancel")
        .methods(crow::HTTPMethod::Patch, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int reservationId)
                                                                     {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", "取消预约");
                    return;
                }

                userHandler handler(dbManager);
                crow::response handlerResponse = handler.cancelReservation(req, userId, reservationId);

                ProcessHandlerResponse(req, res, handlerResponse);

            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "取消预约", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::operation_failed(req, "Failed to cancel reservation", e.what());
            } 
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "取消预约", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 获得订单列表
    CROW_ROUTE(app, "/api/user/orders")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1) {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", "获取订单列表");
                    return;
                }
                orderCommonHandler handler(dbManager);
                crow::response response = handler.getOrderSummary(req, userId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "获取订单列表", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "获取订单列表", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 获得订单信息
    CROW_ROUTE(app, "/api/user/orders/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int orderId)
                                                                   {
            int userId = -1;
            try {
                userId = isValidUserorderToken(req, res, orderId, dbManager);
                
                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", "获取订单详情");
                    return;
                }

                orderCommonHandler handler(dbManager);
                crow::response response = handler.getOrderInformation(req, orderId);
                ProcessHandlerResponse(req, res, response);

            }
            catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "获取订单详情", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "获取订单详情", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    routes_setup = true;
}
