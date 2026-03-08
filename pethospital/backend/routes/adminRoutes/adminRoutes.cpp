#include "adminRoutes.h"

void adminRoutes::setupAdminRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }

    CROW_ROUTE(app, "/admin/getWorkTimeRecord")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([&dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try
            {
                int userId = isValidSuperAdminToken(req, res, dbManager);
                if ( res.code != 200 || userId == -1)
                {
                    return;
                }

                adminHandler handler(dbManager);
                crow::response response = handler.getWorkTimeRecord(req);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception &e)
            {
                res = ResponseHelper::system_error(req);
            }
            res.end(); });

    CROW_ROUTE(app, "/admin/createDoctor")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try
            {
                int userId = isValidSuperAdminToken(req, res, dbManager);
                if(res.code != 200 || userId == -1)         // token验证失败
                {
                    res.end();
                    return;
                }

                adminHandler handler(dbManager);
                crow::response response = handler.createDoctor(req, userId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception &e)
            {
                res = ResponseHelper::system_error(req);
            }
            res.end(); });

    CROW_ROUTE(app, "/admin/deleteDoctor")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::OPTIONS)([&dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try
            {
                int userId = isValidSuperAdminToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    res.end();
                    return;
                }

                adminHandler handler(dbManager);
                crow::response response = handler.deleteDoctor(req, userId);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception &e)
            {
                res = ResponseHelper::system_error(req);
            }
            res.end(); });

    CROW_ROUTE(app, "/admin/changeDoctorWorkTime")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&dbManager](const crow::request &req, crow::response &res)
                                                                    { 
            try 
            {
                int userId = isValidSuperAdminToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    res.end();
                    return;
                }

                adminHandler handler(dbManager);

                // 解析请求体中的 JSON 数据
                auto jsonOpt = handler.parseJson(req, res);
                nlohmann::json& request_body = jsonOpt.value();

                std::string date = request_body["date"].is_string() ? request_body["date"].get<std::string>() : request_body.dump();
                std::string identifier = request_body["identifier"].is_string() ? request_body["identifier"].get<std::string>() : request_body.dump();

                crow::response response = handler.changeDoctorWorkTime(req, userId, date, identifier);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception &e)
            {
                res = ResponseHelper::system_error(req);
            }
            res.end(); });

    routes_setup = true;
}