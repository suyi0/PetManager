#include "UserRoutes.h"

std::unordered_map<std::string, std::chrono::steady_clock::time_point> email_check_last_access;
void UserRoutes::setupUserRoutes(CrowApp& app, DatabaseManagerInterface *dbManager)
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

    // 添加控制验证码准备状态的路由
    CROW_ROUTE(app, "/api/verification/ready")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try {
                UserHandler handler(dbManager);
                crow::response handlerResponse = handler.userReadyVerification(req);
            
                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end(); });

    CROW_ROUTE(app, "/api/user/check/email")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try
            {
                UserHandler handler(dbManager);

                crow::response handlerResponse = handler.userCheckEmail(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end(); });

    //  用户注册验证码验证路由
    CROW_ROUTE(app, "/api/user/verify")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
                try
                {
                    UserHandler handler(dbManager);
                    crow::response handlerResponse = handler.userVerification(req);
                
                    ProcessHandlerResponse(req, res, handlerResponse);
                } catch (const std::exception& e) {
                    res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
                }
                res.end(); });

    // 添加保存表单数据的路由
    CROW_ROUTE(app, "/api/user/form")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                                                                     {
            try
            {   
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
    CROW_ROUTE(app, "/api/user/upload/avatar")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try
            {
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
    CROW_ROUTE(app, "/api/user/data")
        .methods(crow::HTTPMethod::Get)([dbManager](const crow::request &req, crow::response &res)
                                        {
            try
            {
                UserHandler handler(dbManager);
                crow::response handlerResponse = handler.getUserdata(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception &e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end(); });

    routes_setup = true;
}