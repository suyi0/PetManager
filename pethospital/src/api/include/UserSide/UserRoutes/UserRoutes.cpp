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
                crow::response login_response = handler.userLogin(req);

                // 安全解析JSON
                nlohmann::json response_json;
                if (!login_response.body.empty())
                {
                    try {
                        response_json = nlohmann::json::parse(login_response.body);
                    } catch (const std::exception& e) {
                        // 如果解析失败，使用原始响应
                        res.code = login_response.code;
                        res.body = login_response.body;
                        initializeCORS(req, res);
                        res.end();
                        return;
                    }
                }
                res = ResponseHelper::custom(req, login_response.code, response_json);
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
                crow::response readyVerify_response = handler.userReadyVerification(req);
            
                nlohmann::json response_json;
                if (!readyVerify_response.body.empty()) {
                    try {
                        response_json = nlohmann::json::parse(readyVerify_response.body);
                    } catch (...) {
                        res.code = readyVerify_response.code;
                        res.body = readyVerify_response.body;
                        initializeCORS(req, res);
                        res.end();
                        return;
                    }
                }

                res = ResponseHelper::custom(req, readyVerify_response.code, response_json);
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
                    crow::response verify_response = handler.userVerification(req);
                
                    nlohmann::json response_json;
                    if (!verify_response.body.empty()) {
                    try {
                        response_json = nlohmann::json::parse(verify_response.body);
                    } catch (...) {
                        res.code = verify_response.code;
                        res.body = verify_response.body;
                        initializeCORS(req, res);
                        res.end();
                        return;
                    }
                    res = ResponseHelper::custom(req, verify_response.code, response_json);
                    }
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
                crow::response update_response = handler.userUpdate(req);

                nlohmann::json response_json;
                if (!update_response.body.empty())
                {
                    try
                    {
                        response_json = nlohmann::json::parse(update_response.body);
                    }
                    catch (...)
                    {
                        res.code = update_response.code;
                        res.body = update_response.body;
                        initializeCORS(req, res);
                        res.end();
                        return;
                    }
                }
                res = ResponseHelper::custom(req, update_response.code, response_json);
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
                crow::response uploadAvatar_response = handler.userUploadAvatar(req);

                nlohmann::json response_json;
                if (!uploadAvatar_response.body.empty())
                {
                    try
                    {
                        response_json = nlohmann::json::parse(uploadAvatar_response.body);
                    }
                    catch (...)
                    {
                        res.code = uploadAvatar_response.code;
                        res.body = uploadAvatar_response.body;
                        initializeCORS(req, res);
                        res.end();
                        return;
                    }
                }
                res = ResponseHelper::custom(req, uploadAvatar_response.code, response_json);
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
                crow::response file_response = handler.upload(req, filename);

                nlohmann::json response_json;
                if (!file_response.body.empty())
                {
                    try
                    {
                        response_json = nlohmann::json::parse(file_response.body);
                    } catch (...)
                    {
                        res.code = file_response.code;
                        res.body = file_response.body;
                        initializeCORS(req, res);
                        res.end();
                        return;
                    }
                }

                res = ResponseHelper::custom(req, file_response.code, response_json);
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
                crow::response userdata_response = handler.getUserdata(req);

                nlohmann::json response_json;
                if (!userdata_response.body.empty())
                {
                    try
                    {
                        response_json = nlohmann::json::parse(userdata_response.body);
                    } catch (...)
                    {
                        res.code = userdata_response.code;
                        res.body = userdata_response.body;
                        initializeCORS(req, res);
                        res.end();
                        return;
                    }
                }

                res = ResponseHelper::custom(req, userdata_response.code, response_json);
            } catch (const std::exception &e)
            {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();
        });

    routes_setup = true;
}