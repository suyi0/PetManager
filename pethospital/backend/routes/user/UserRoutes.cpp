#include "UserRoutes.h"
#include "../../controllers/OperationLogger/OperationLogger.h"

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
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "账号登录", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
                                                        
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "账号登录"); });

    // 添加保存表单数据路由
    CROW_ROUTE(app, "/api/upload/form")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Put, crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                                                                     {
            int userId = -1;
            try
            {   
                // 解析令牌信息，确认用户身份和权限
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", "更新资料");
                    return;
                }
                UserHandler handler(dbManager);
                crow::response handlerResponse = handler.userUpdate(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "更新资料", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }

            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "更新资料", userId > 0 ? std::optional<int>(userId) : std::nullopt); });
    // 上传头像
    // 文件上传请求,客户端发送的是 multipart/form-data 格式，不是 JSON 格式
    CROW_ROUTE(app, "/api/upload/avatar")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([dbManager](const crow::request &req, crow::response &res)
                                                                    {
            int userId = -1;
            try
            {
                userId = isValidUserToken(req, res, dbManager);

                if(res.code != 200 || userId == -1)
                {
                    OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "用户", "上传头像");
                    return;
                }
                UserHandler handler(dbManager);
                crow::response handlerResponse = handler.userUploadAvatar(req);

                ProcessHandlerResponse(req, res, handlerResponse);
            } catch (const std::exception& e)
            {
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "上传头像", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "上传头像", userId > 0 ? std::optional<int>(userId) : std::nullopt); });
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
                OperationLogger::LogExceptionOperation(dbManager, req, "用户", "获取上传文件", e.what());
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
            OperationLogger::FinishLoggedRoute(dbManager, req, res, "用户", "获取上传文件", std::nullopt, false); });

    routes_setup = true;
}
