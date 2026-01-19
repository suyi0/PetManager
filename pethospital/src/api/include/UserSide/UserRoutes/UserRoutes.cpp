#include "UserRoutes.h"

void UserRoutes::UserRoutes::setupUserRoutes(crow::SimpleApp &app, DatabaseManagerInterface *dbManager)
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
        return;

    // 创建 UserHandler 实例
    UserHandler handler(dbManager);

    // 添加登录路由
    CROW_ROUTE(app, "/api/user/login")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                    {
            // 处理OPTIONS预检请求
            initializeOPTIONS(req,res);
            if(res.is_completed())
            {
                return; // 如果是OPTIONS请求，直接返回
            }
            try {
                crow::response handlerResponse = handler.userLogin(req);

                // 安全解析JSON
                ProcessHandlerResponse(req, res, handlerResponse);
            } catch(const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
                                                        
            res.end();
        });

    // 添加根路径测试路由
    CROW_ROUTE(app, "/")
    ([]()
     { return "Server is running!"; });

    // 添加控制验证准备状态的路由
    CROW_ROUTE(app, "/api/verification/ready")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                    {
            // 处理OPTIONS请求
            initializeOPTIONS(req, res);
            if (res.is_completed())
            {
                return; // 如果是OPTIONS请求，直接返回
            }

            try {
                crow::response handlerResponse = handler.userReadyVerification(req);
            
                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end(); });

    //  用户注册验证码验证路由
    CROW_ROUTE(app, "/api/user/verify")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                    {
                // 处理OPTIONS预检请求
                initializeOPTIONS(req,res);
        
                if (res.is_completed())
                {
                    return; // 如果是OPTIONS请求，直接返回
                }

                try
                {
                    crow::response handlerResponse = handler.userVerification(req);
                
                    ProcessHandlerResponse(req, res, handlerResponse);
                } catch (const std::exception& e) {
                    res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
                }
                res.end(); });

    // 添加保存表单数据的路由
    CROW_ROUTE(app, "/api/user/form")
        .methods(crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                      {
            // 处理OPTIONS预检请求
            initializeOPTIONS(req, res);

            if (res.is_completed())
            {
                return; // 如果是OPTIONS请求，直接返回
            }
            try
            {
                crow::response handlerResponse = handler.userUpdate(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }

            res.end();
        });
    // 上传头像
    CROW_ROUTE(app, "/api/user/upload/avatar")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([&handler](const crow::request &req, crow::response &res)
                                                                    {
            // 处理OPTIONS预检请求
            initializeOPTIONS(req,res);
            if (res.is_completed())
            {
                return; // 如果是OPTIONS请求，直接返回
            }
            try
            {
                crow::response handlerResponse = handler.userUploadAvatar(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();
            });
    // 添加静态文件服务路由
    CROW_ROUTE(app, "/uploads/<string>")
        .methods(crow::HTTPMethod::Get)([&handler](const crow::request &req, crow::response &res, std::string filename)
                                        {
            // 处理OPTIONS预检请求
            initializeOPTIONS(req,res);
            if (res.is_completed())
            {
                return; // 如果是OPTIONS请求，直接返回
            }
            try
            {
                crow::response handlerResponse = handler.upload(req, filename);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception &e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();
        });
    // 获取数据库数据的路由
    CROW_ROUTE(app, "/api/user/data")
        .methods(crow::HTTPMethod::Get)([&handler](const crow::request &req, crow::response &res)
                                        {
            initializeCORS(req, res);
             if (res.is_completed())
            {
                return; // 如果是OPTIONS请求，直接返回
            }
            try
            {
                crow::response handlerResponse = handler.getUserdata(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception &e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();
        });

    routes_setup = true;
}