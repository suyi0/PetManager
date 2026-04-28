#include "authRoutes.h"
#include "../../controllers/OperationLogger/OperationLogger.h"

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
                OperationLogger::LogExceptionOperation(dbManager, req, "认证", "检查用户名", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "认证", "检查用户名", std::nullopt, false);
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
                OperationLogger::LogExceptionOperation(dbManager, req, "认证", "检查邮箱", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "认证", "检查邮箱", std::nullopt, false);
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
                OperationLogger::LogExceptionOperation(dbManager, req, "认证", "检查手机号", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "认证", "检查手机号", std::nullopt, false);
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
                OperationLogger::LogExceptionOperation(dbManager, req, "认证", "准备验证码", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "认证", "准备验证码", std::nullopt, false);
        });

    // 添加控制验证码验证的路由
    CROW_ROUTE(app, "/api/verification/email/verify")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
                try
                {
                    authHandler handler(dbManager);
                    crow::response handlerResponse = handler.checkVerifyEmailCode(req);
                
                    ProcessHandlerResponse(req, res, handlerResponse);
                } catch (const std::exception& e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "认证", "邮箱登录", e.what());
                    res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "认证", "邮箱登录"); 
        });

    CROW_ROUTE(app, "/api/auth/admin/refresh")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
                int userId = -1;
                try
                {
                    userId = isValidManagementToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "认证", "刷新管理员令牌");
                        return;
                    }
                    authHandler handler(dbManager);
                    crow::response handlerResponse = handler.refreshAdminToken(req);

                    ProcessHandlerResponse(req, res, handlerResponse);
                } catch (const std::exception& e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "认证", "刷新管理员令牌", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "认证", "刷新管理员令牌", userId > 0 ? std::optional<int>(userId) : std::nullopt);
        });

        CROW_ROUTE(app, "/api/verification/sms/send")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try {
                authHandler handler(dbManager);
                crow::response handlerResponse = handler.sendSmsVerification(req);
            
                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "认证", "发送短信验证码", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "认证", "发送短信验证码", std::nullopt, false);
        });

        CROW_ROUTE(app, "/api/verification/sms/verify")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            try {
                authHandler handler(dbManager);
                crow::response handlerResponse = handler.checkVerifySmsCode(req);
            
                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e) {
                OperationLogger::LogExceptionOperation(dbManager, req, "认证", "校验短信验证码", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "认证", "校验短信验证码", std::nullopt, false);
        });
}
