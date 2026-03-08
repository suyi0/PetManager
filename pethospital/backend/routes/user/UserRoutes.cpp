#include "UserRoutes.h"

std::unordered_map<std::string, std::chrono::steady_clock::time_point> email_check_last_access;
void UserRoutes::setupUserRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
        return;

    // 添加登录路由
    CROW_ROUTE(app, "/api/user/login")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try {
                UserHandler handler(dbManager);
                crow::response handlerResponse = handler.userLogin(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch(const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
                                                        
            res.end(); });

    // 添加根路径测试路由
    CROW_ROUTE(app, "/")
    ([]()
     { return "Server is running!"; });

    // 添加保存表单数据路由
    CROW_ROUTE(app, "/api/upload/form")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                                                                     {
            try
            {   
                isValidUserToken(req, res, dbManager);

                if(res.code != 200)
                {
                    res.end();
                    return;
                }
                UserHandler handler(dbManager);
                crow::response handlerResponse = handler.userUpdate(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }

            res.end(); });
    // 上传头像
    // 文件上传请求,客户端发送的是 multipart/form-data 格式，不是 JSON 格式
    CROW_ROUTE(app, "/api/upload/avatar")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try
            {
                isValidUserToken(req, res, dbManager);

                if(res.code != 200)
                {
                    res.end();
                    return;
                }
                UserHandler handler(dbManager);
                crow::response handlerResponse = handler.userUploadAvatar(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end(); });
    // 添加静态文件服务路由
    CROW_ROUTE(app, "/uploads/<string>")
        .methods(crow::HTTPMethod::Get)([dbManager](const crow::request &req, crow::response &res, std::string filename)
                                        {
            try
            {
                UserHandler handler(dbManager);
                crow::response handlerResponse = handler.upload(req, filename);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception &e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end(); });

    // 获取数据库数据的路由
    // 只有管理员可以获取所有用户数据
    CROW_ROUTE(app, "/api/allUser/getdata")
        .methods(crow::HTTPMethod::Get)([dbManager](const crow::request &req, crow::response &res)
                                        {
            try
            {
                isValidUserToken(req, res, dbManager);

                if(res.code != 200)
                {
                    res.end();
                    return;
                }
                UserHandler handler(dbManager);
                crow::response handlerResponse = handler.getData(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception &e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end(); });

    routes_setup = true;
}