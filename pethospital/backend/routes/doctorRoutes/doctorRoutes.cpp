#include "doctorRoutes.h"
#include "../../controllers/auth/jwtUtils/jwtUtils.h"
#include "../../services/logger/operationLogger.h"
#include "../../services/realtime/doctorBroadcaster/doctorQueueBroadcaster.h"
#include "../../services/realtime/medicineBroadcaster/medicineStockBroadcaster.h"
#include "../../services/auth/AuthSessionStore.h"
#include "../../utils/permissions/Permissions.h"

#include <iostream>

void DoctorRoutes::setupDoctorRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }

    // 获取医生本人信息路由.
    CROW_ROUTE(app, "/api/doctors/profile")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
                int userId = -1;
                try
                {
                    userId = isValidMedicalStaffToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取医生信息");
                        return;
                    }
                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.getDoctor(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "医生", "获取医生信息", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取医生信息", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 获取用户列表路由.
    CROW_ROUTE(app, "/api/doctors/user-summaries")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
                int userId = -1;
                try
                {
                    userId = isValidMedicalStaffToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取用户列表");
                        return;
                    }

                    doctorHandler doctorHandler(dbManager);
                    auto jsonOpt = doctorHandler.parseJson(req, res);
                    if (!jsonOpt) {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取用户列表", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    nlohmann::json &request_body = jsonOpt.value();
                    std::string data = request_body.contains("data") ? request_body["data"] : "";
                    std::string identifier = request_body.contains("identifier") ? request_body["identifier"] : "";

                    crow::response response = doctorHandler.getUserList(req, data, identifier);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "医生", "获取用户列表", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取用户列表", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 获取可开单药品列表路由.
    CROW_ROUTE(app, "/api/doctors/medicines")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
                int userId = -1;
                try
                {
                    userId = isValidMedicalStaffToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取药品列表");
                        return;
                    }

                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.getMedicineList(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "医生", "获取药品列表", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取药品列表", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 搜索可开单药品路由.
    CROW_ROUTE(app, "/api/doctors/medicine-search")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
                int userId = -1;
                try
                {
                    userId = isValidMedicalStaffToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "搜索药品");
                        return;
                    }

                    doctorHandler doctorHandler(dbManager);
                    auto jsonOpt = doctorHandler.parseJson(req, res);
                    if (!jsonOpt) {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "搜索药品", userId > 0 ? std::optional<int>(userId) : std::nullopt);
                        return;
                    }

                    nlohmann::json &request_body = jsonOpt.value();
                    std::string keyword;
                    if (request_body.contains("keyword") && request_body["keyword"].is_string()) {
                        keyword = request_body["keyword"].get<std::string>();
                    }

                    crow::response response = doctorHandler.searchMedicines(req, keyword);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "医生", "搜索药品", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "搜索药品", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 获取用户档案信息路由.
    CROW_ROUTE(app, "/api/doctors/user-profiles/<int>")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res, int goalUserId)
                                                                   {
                int userId = -1;
                try
                {
                    userId = isValidMedicalStaffToken(req, res, dbManager);

                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取用户档案");
                        return;
                    }

                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.getUserProfiles(req, goalUserId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "医生", "获取用户档案", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取用户档案", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 获取值班状态路由.
    CROW_ROUTE(app, "/api/doctors/duty-status")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
                int userId = -1;
                try
                {
                    userId = isValidMedicalStaffToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取值班状态");
                        return;
                    }
                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.getDutyStatus(req, userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "医生", "获取值班状态", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取值班状态", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 获取待接诊队列路由.
    CROW_ROUTE(app, "/api/doctors/queues")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
                int userId = -1;
                try
                {
                    userId = isValidMedicalStaffToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取待接诊队列");
                        return;
                    }

                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.getQueues(req, userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "医生", "获取待接诊队列", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取待接诊队列", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 医生端待接诊队列实时通道。
    CROW_WEBSOCKET_ROUTE(app, "/realtime/doctors/queues")
        .onaccept([dbManager](const crow::request &req, void **userdata)
                  {
            const char *tokenParam = req.url_params.get("token");
            if (tokenParam == nullptr || std::string(tokenParam).empty())
            {
                return false;
            }

            auto claims = JwtUtils::getTokenClaims(tokenParam);
            if (!claims || claims->userId <= 0 || !dbManager || !dbManager->getSession())
            {
                return false;
            }

            // 与其它实时通道一致：失效会话不再建立连接（医生为非管理角色时此校验是安全 no-op）。
            if (!AuthSessionStore::isSessionCurrent(claims->userId, claims->typeName, claims->sessionVersion))
            {
                return false;
            }

            std::string identifier = claims->identifier;
            if (!JwtUtils::isUserAuthorizedForMedicalStaffForm(
                    claims->userId,
                    identifier,
                    claims->isEmailLogin,
                    dbManager))
            {
                return false;
            }

            *userdata = new int(claims->userId);
            return true; })
        .onopen([](crow::websocket::connection &conn)
                {
            auto *doctorId = static_cast<int *>(conn.userdata());
            if (!doctorId || *doctorId <= 0)
            {
                conn.close("invalid_doctor_queue_session");
                return;
            }

            DoctorQueueBroadcaster::instance().addConnection(&conn, *doctorId);
            delete doctorId;
            conn.userdata(nullptr); })
        .onclose([](crow::websocket::connection &conn, const std::string &, uint16_t)
                 {
            DoctorQueueBroadcaster::instance().removeConnection(&conn); })
        .onerror([](crow::websocket::connection &conn, const std::string &reason)
                 {
            std::cerr << "Doctor queue WebSocket error: " << reason << std::endl;
            DoctorQueueBroadcaster::instance().removeConnection(&conn); });

    // 医生端药品库存实时通道：任一处库存变更后广播刷新信号，前端据此重拉药品列表。
    CROW_WEBSOCKET_ROUTE(app, "/realtime/doctors/medicine-stock")
        .onaccept([dbManager](const crow::request &req, void **)
                  {
            const char *tokenParam = req.url_params.get("token");
            if (tokenParam == nullptr || std::string(tokenParam).empty())
            {
                return false;
            }

            auto claims = JwtUtils::getTokenClaims(tokenParam);
            if (!claims || claims->userId <= 0 || !dbManager || !dbManager->getSession())
            {
                return false;
            }

            // 与其它实时通道一致：失效会话不再建立连接。
            if (!AuthSessionStore::isSessionCurrent(claims->userId, claims->typeName, claims->sessionVersion))
            {
                return false;
            }

            std::string identifier = claims->identifier;
            return JwtUtils::isUserAuthorizedForMedicalStaffForm(
                claims->userId,
                identifier,
                claims->isEmailLogin,
                dbManager); })
        .onopen([](crow::websocket::connection &conn)
                {
            MedicineStockBroadcaster::instance().addConnection(&conn); })
        .onclose([](crow::websocket::connection &conn, const std::string &, uint16_t)
                 {
            MedicineStockBroadcaster::instance().removeConnection(&conn); })
        .onerror([](crow::websocket::connection &conn, const std::string &reason)
                 {
            std::cerr << "Medicine stock WebSocket error: " << reason << std::endl;
            MedicineStockBroadcaster::instance().removeConnection(&conn); });

    // 获取预约摘要路由.
    CROW_ROUTE(app, "/api/doctors/reservation-summaries")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
        int userId = -1;
        try
        {
            userId = isValidMedicalStaffToken(req, res, dbManager);

            if (res.code != 200 || userId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取预约摘要");
                return;
            }
            reservationCommonHandler handler(dbManager);
            crow::response handlerResponse = handler.getReservationSummary(req, userId);

            ProcessHandlerResponse(req, res, handlerResponse);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", "获取预约摘要", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            res = ResponseHelper::operation_failed(req, "Failed to fetch reservation summary", e.what());
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取预约摘要", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 获取预约订单信息路由.
    CROW_ROUTE(app, "/api/doctors/reservations/<int>/information")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res, int reservationId)
                                                                   {
        int userId = -1;
        try
        {
            userId = isValidMedicalStaffToken(req, res, dbManager);

            if (res.code != 200 || userId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取预约订单");
                return;
            }

            reservationCommonHandler reservationHandler(dbManager);
            crow::response response = reservationHandler.getReservationInformation(req, reservationId);
            ProcessHandlerResponse(req, res, response);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", "获取预约订单", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            res = ResponseHelper::system_error(req);
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取预约订单", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 更新预约状态路由.
    CROW_ROUTE(app, "/api/doctors/reservations/<int>/statuses")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res, int reservationId)
                                                                    {
        int userId = -1;
        try
        {
            userId = isValidMedicalStaffToken(req, res, dbManager);

            if (res.code != 200 || userId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "更新预约状态");
                return;
            }

            doctorHandler doctorHandler(dbManager);
            crow::response response = doctorHandler.updateReservationStatus(req, userId, reservationId);
            ProcessHandlerResponse(req, res, response);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", "更新预约状态", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            res = ResponseHelper::system_error(req);
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "更新预约状态", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 创建诊单记录路由.
    CROW_ROUTE(app, "/api/doctors/order-records")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
        int userId = -1;
        try
        {
            userId = isValidPermissionToken(req, res, dbManager, Permissions::kMedicalRecordWrite);

            if (res.code != 200 || userId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "创建诊单记录");
                return;
            }

            doctorHandler doctorHandler(dbManager);
            crow::response response = doctorHandler.createOrderRecord(req, userId);
            ProcessHandlerResponse(req, res, response);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", "创建诊单记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            res = ResponseHelper::system_error(req);
        }
        OperationLogger::FinishSensitiveRoute(dbManager, req, res, "医生", "创建诊单记录", Permissions::kMedicalRecordWrite, userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 获取诊单摘要路由.
    CROW_ROUTE(app, "/api/doctors/order-summaries")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res)
                                                                   {
        int userId = -1;
        try
        {
            userId = isValidMedicalStaffToken(req, res, dbManager);

            if (res.code != 200 || userId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取诊单摘要");
                return;
            }

            orderCommonHandler orderHandler(dbManager);
            crow::response response = orderHandler.getOrderSummary(req, userId);
            ProcessHandlerResponse(req, res, response);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", "获取诊单摘要", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            res = ResponseHelper::system_error(req);
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取诊单摘要", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 获取诊单详情路由.
    CROW_ROUTE(app, "/api/doctors/orders/<int>/information")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res, int orderId)
                                                                   {
        int userId = -1;
        try
        {
            userId = isValidMedicalStaffToken(req, res, dbManager);

            if (res.code != 200 || userId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取诊单详情");
                return;
            }
            if (!JwtUtils::isUserAuthorizedForOrder(userId, orderId, dbManager))
            {
                res = ResponseHelper::notFound(req, "Order not found");
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "获取诊单详情");
                return;
            }

            orderCommonHandler orderHandler(dbManager);
            crow::response response = orderHandler.getOrderInformation(req, orderId);
            ProcessHandlerResponse(req, res, response);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", "获取诊单详情", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            res = ResponseHelper::system_error(req);
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取诊单详情", userId > 0 ? std::optional<int>(userId) : std::nullopt, false); });

    // 医生端管理指定用户的宠物档案路由
    CROW_ROUTE(app, "/api/doctors/user-profiles/<int>/pet-profiles")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int targetUserId)
                                                                                           {
        int doctorId = -1;
        try
        {
            doctorId = isValidMedicalStaffToken(req, res, dbManager);
            if (res.code != 200 || doctorId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "管理宠物档案");
                return;
            }

            petCommonHandler handler(dbManager);
            crow::response handlerResponse =
                req.method == crow::HTTPMethod::Post
                    ? handler.createPetProfile(req, targetUserId)
                    : handler.getPetProfiles(req, targetUserId);

            ProcessHandlerResponse(req, res, handlerResponse);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", "管理宠物档案", e.what(), doctorId > 0 ? std::optional<int>(doctorId) : std::nullopt);
            res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "管理宠物档案", doctorId > 0 ? std::optional<int>(doctorId) : std::nullopt, req.method != crow::HTTPMethod::Get); });

    // 医生端更新/删除指定用户的宠物档案路由
    CROW_ROUTE(app, "/api/doctors/user-profiles/<int>/pet-profiles/<int>")
        .methods(crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int targetUserId, int petId)
                                                                                             {
        int doctorId = -1;
        try
        {
            doctorId = isValidMedicalStaffToken(req, res, dbManager);
            if (res.code != 200 || doctorId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "管理宠物档案");
                return;
            }

            petCommonHandler handler(dbManager);
            crow::response handlerResponse =
                req.method == crow::HTTPMethod::Delete
                    ? handler.deletePetProfile(req, targetUserId, petId)
                    : handler.updatePetProfile(req, targetUserId, petId);

            ProcessHandlerResponse(req, res, handlerResponse);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", "管理宠物档案", e.what(), doctorId > 0 ? std::optional<int>(doctorId) : std::nullopt);
            res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "管理宠物档案", doctorId > 0 ? std::optional<int>(doctorId) : std::nullopt); });

    // 医生打卡接口
    CROW_ROUTE(app, "/api/doctors/duty-status-changes")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
        int userId = -1;
        try
        {
            userId = isValidMedicalStaffToken(req, res, dbManager);

            if (res.code != 200 || userId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "更新值班状态");
                return;
            }
            doctorHandler doctorHandler(dbManager);
            crow::response response = doctorHandler.handleDutyAction(req, userId, false);
            ProcessHandlerResponse(req, res, response);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", "更新值班状态", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            res = ResponseHelper::system_error(req);
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "更新值班状态", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 修改订单
    CROW_ROUTE(app, "/api/doctors/orders/<int>/changes")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res, int orderId)
                                                                   {
        int userId = -1;
        try
        {
            // 验证用户token权限
            userId = isValidUserorderToken(req, res, orderId, dbManager);

            if (res.code != 200 || userId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "订单", "修改订单");
                return;
            }

            // 验证成功进行订单处理
            doctorHandler handler(dbManager);
            crow::response response = handler.changeOrder(req, orderId, userId);
            ProcessHandlerResponse(req, res, response);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "订单", "修改订单", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "订单", "修改订单", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 搜索关键字获取对应记录
    CROW_ROUTE(app, "/api/doctors/search-keyword")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
        int userId = -1;
        try
        {
            userId = isValidMedicalStaffToken(req, res, dbManager);

            if (res.code != 200 || userId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "搜索关键字获取对应记录");
                return;
            }

            searchCommonHandler handler(dbManager);
            crow::response response = handler.searchByKeyword(req, userId);
            ProcessHandlerResponse(req, res, response);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", "搜索关键字获取对应记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "搜索关键字获取对应记录", userId > 0 ? std::optional<int>(userId) : std::nullopt); });

    // 搜索历史记录
    CROW_ROUTE(app, "/api/doctors/search-history")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                                           {
        if (req.method == crow::HTTPMethod::Options)
        {
            res.code = 204;
            return;
        }

        int userId = -1;
        const bool isUpdate = req.method == crow::HTTPMethod::Post;
        const std::string actionName = isUpdate ? "更新搜索历史记录" : "获取搜索历史记录";
        try
        {
            userId = isValidMedicalStaffToken(req, res, dbManager);

            if (res.code != 200 || userId == -1)
            {
                OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", actionName);
                return;
            }

            searchCommonHandler handler(dbManager);
            crow::response response = isUpdate ? handler.searchDataUpdate(req, userId) : handler.getSearchHistoryData(req, userId);
            ProcessHandlerResponse(req, res, response);
        }
        catch (const std::exception &e)
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "医生", actionName, e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
        }
        OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", actionName, userId > 0 ? std::optional<int>(userId) : std::nullopt, isUpdate); });

    routes_setup = true;
}
