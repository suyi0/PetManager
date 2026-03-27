#include "doctorRoutes.h"

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
                try
                {
                    int userId = isValidUserToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        res.end();
                        return;
                    }
                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.getDoctor(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    res = ResponseHelper::system_error(req);
                }
                res.end();
            });

    CROW_ROUTE(app,"/api/doctor/createUser")
            .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request& req, crow::response& res){ 
                try
                {
                    int userId = isValidUserToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        res.end();
                        return;
                    }

                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.createUser(req);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    res = ResponseHelper::system_error(req);
                }
            });
    
    CROW_ROUTE(app,"/api/doctor/getUserList")
            .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request& req, crow::response& res){
                try
                {
                    int userId = isValidUserToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        res.end();
                        return;
                    }

                    doctorHandler doctorHandler(dbManager);
                    auto jsonOpt = doctorHandler.parseJson(req, res);
                    if (!jsonOpt) {
                        res.end();
                        return;
                    }

                    nlohmann::json &request_body = jsonOpt.value();
                    std::string data = request_body.contains("data") ? request_body["data"] : "";
                    std::string identifier = request_body.contains("identifier") ? request_body["identifier"] : "";

                    crow::response response = doctorHandler.getUserList(req, data, identifier);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    res = ResponseHelper::system_error(req);
                }
            });

    CROW_ROUTE(app,"/api/doctor/dutyStatus")
            .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request& req, crow::response& res){
                try
                {
                    int userId = isValidUserToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        res.end();
                        return;
                    }
                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.getDutyStatus(req, userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &e) {
                    res = ResponseHelper::system_error(req);
                }
                res.end();
            });

    CROW_ROUTE(app, "/api/doctor/online")
            .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager] (const crow::request& req, crow::response& res) {
                try{
                    int userId = isValidUserToken(req, res, dbManager);

                    if(res.code != 200 || userId == -1)
                    {
                        res.end();
                        return;
                    }
                    doctorHandler doctorHandler(dbManager);
                    crow::response response = doctorHandler.onlineDoctor(req, userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception& e) {
                    res = ResponseHelper::system_error(req);
                }
                res.end();
            });
    CROW_ROUTE(app, "/api/doctor/offline")
            .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager] (const crow::request& req, crow::response& res) {
              try{
                int userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    res.end();
                    return;
                }
                doctorHandler doctorHandler(dbManager);
                crow::response respone = doctorHandler.offlineDoctor(req, userId);
                ProcessHandlerResponse(req, res, respone);
              }
              catch (const std::exception& e)
              {
                res = ResponseHelper::system_error(req);
              }
              res.end();
            });

    routes_setup = true;
}
