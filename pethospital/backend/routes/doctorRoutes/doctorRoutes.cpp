#include "doctorRoutes.h"
#include "../../controllers/OperationLogger/OperationLogger.h"

void DoctorRoutes::setupDoctorRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }
    
    CROW_ROUTE(app,"/api/doctor/getDoctor")
            .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request& req, crow::response& res){
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
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取医生信息", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app,"/api/doctor/createUser")
            .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request& req, crow::response& res){ 
                int userId = -1;
                try
                {
                    userId = isValidMedicalStaffToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "创建用户");
                        return;
                    }

                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.createUser(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "医生", "创建用户", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "创建用户", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });
    
    CROW_ROUTE(app,"/api/doctor/getUserList")
            .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request& req, crow::response& res){
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
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取用户列表", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app,"/api/doctor/dutyStatus")
            .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request& req, crow::response& res){
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
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "获取值班状态", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/doctor/dutyStatus/action")
            .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager] (const crow::request& req, crow::response& res) {
                int userId = -1;
                try{
                    userId = isValidMedicalStaffToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "医生", "更新值班状态");
                        return;
                    }
                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.handleDutyAction(req, userId, false);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception& e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "医生", "更新值班状态", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "医生", "更新值班状态", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    routes_setup = true;
}
