#include "authRoutes.h"

void authRoutes::setupAuthRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }
    routes_setup = true;

    CROW_ROUTE(app, "/api/auth/checkName")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try
            {
                authHandler handler(dbManager);

                crow::response handlerResponse = handler.authCheckName(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();
        });

    CROW_ROUTE(app, "/api/auth/checkEmail")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try
            {
                authHandler handler(dbManager);

                crow::response handlerResponse = handler.authCheckEmail(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();
        });

    CROW_ROUTE(app, "/api/auth/checkPhone")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try
            {
                authHandler handler(dbManager);

                crow::response handlerResponse = handler.authCheckPhone(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();
        });

    // 添加控制验证码准备状态的路由
    CROW_ROUTE(app, "/api/verification/ready")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try {
                authHandler handler(dbManager);
                crow::response handlerResponse = handler.authReadyVerification(req);
            
                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();
        });

    // 添加控制验证码验证的路由
    CROW_ROUTE(app, "/api/auth/verify")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
                try
                {
                    authHandler handler(dbManager);
                    crow::response handlerResponse = handler.authVerification(req);
                
                    ProcessHandlerResponse(req, res, handlerResponse);
                } catch (const std::exception& e) {
                    res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
                }
                res.end(); 
        });

    CROW_ROUTE(app, "/api/auth/admin/refresh")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
                try
                {
                    authHandler handler(dbManager);
                    crow::response handlerResponse = handler.refreshAdminToken(req);

                    ProcessHandlerResponse(req, res, handlerResponse);
                } catch (const std::exception& e) {
                    res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
                }
                res.end();
        });

        CROW_ROUTE(app, "/api/verification/sms/send")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try {
                authHandler handler(dbManager);
                crow::response handlerResponse = handler.sendSmsVerification(req);
            
                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();
        });

        CROW_ROUTE(app, "/api/verification/sms/verify")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try {
                authHandler handler(dbManager);
                crow::response handlerResponse = handler.checkVerifySmsCode(req);
            
                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            res.end();
        });
}
