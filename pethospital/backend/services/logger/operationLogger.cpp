#include "operationLogger.h"
#include "../realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../utils/requestUtils/RequestUtils.h"

namespace
{
    // 当调用方没有显式传摘要时，回退到动作名，避免日志摘要为空。
    std::string fallbackSummary(const std::string &summary, const std::string &action)
    {
        return summary.empty() ? action : summary;
    }

    // 统一补齐来源字段，便于后续按接口来源或任务来源筛选日志。
    std::string fallbackSource(const std::string &source)
    {
        return source.empty() ? "api" : source;
    }

    // 将 Crow 的枚举方法转成稳定字符串，便于直接写入日志详情和来源。
    std::string getMethodName(crow::HTTPMethod method)
    {
        switch (method)
        {
        case crow::HTTPMethod::Get:
            return "GET";
        case crow::HTTPMethod::Post:
            return "POST";
        case crow::HTTPMethod::Put:
            return "PUT";
        case crow::HTTPMethod::Delete:
            return "DELETE";
        case crow::HTTPMethod::Patch:
            return "PATCH";
        case crow::HTTPMethod::Options:
            return "OPTIONS";
        default:
            return "UNKNOWN";
        }
    }

    // 结果日志按 HTTP 状态码粗分为成功、警告、失败，保持审计维度统一。
    std::string getOperationResult(const crow::response &res)
    {
        if (res.code >= 200 && res.code < 300)
        {
            return "成功";
        }
        if (res.code >= 500)
        {
            return "失败";
        }
        return "警告";
    }

    // 响应体优先解析为 JSON，解析失败时保留原始文本，避免日志丢失上下文。
    nlohmann::json parseResponseBody(const crow::response &res)
    {
        if (res.body.empty())
        {
            return nlohmann::json::object();
        }

        try
        {
            return nlohmann::json::parse(res.body);
        }
        catch (...)
        {
            return nlohmann::json{{"rawBody", res.body}};
        }
    }

    // 摘要尽量短，优先使用响应 message，没有再退回到模块/动作/状态码。
    std::string buildOperationSummary(const std::string &module,
                                      const std::string &action,
                                      const crow::response &res,
                                      const nlohmann::json &responseBody)
    {
        std::string message;
        if (responseBody.contains("message") && responseBody["message"].is_string())
        {
            message = responseBody["message"].get<std::string>();
        }

        if (!message.empty())
        {
            return module + "-" + action + ": " + message;
        }

        return module + "-" + action + " [" + std::to_string(res.code) + "]";
    }

    // 获取Json对象中指定字段的值
    std::string getJsonStringField(const nlohmann::json &json,
                                   const std::string &field)
    {
        if (json.contains(field) && json[field].is_string())
        {
            return json[field].get<std::string>();
        }
        return "";
    }

    std::optional<int> getUserIdFromRequestToken(const crow::request &req)
    {
        const std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
        {
            return std::nullopt;
        }

        const std::string token = authHeader.substr(7);
        if (token.empty())
        {
            return std::nullopt;
        }

        auto claims = JwtUtils::getTokenClaims(token);
        if (!claims || claims->userId <= 0)
        {
            return std::nullopt;
        }

        return claims->userId;
    }

    // 详情保存排障所需的最小闭环信息，避免每个路由重复拼装。
    std::string buildOperationDetails(const crow::request &req,
                                      const crow::response &res,
                                      const nlohmann::json &responseBody)
    {
        nlohmann::json details = {
            {"method", getMethodName(req.method)},
            {"path", req.url},
            {"httpStatus", res.code},
            {"clientIp", RequestUtils::getClientIp(req)},
            {"response", responseBody}};

        return details.dump();
    }

    // 统一封装用户/系统日志分流，避免结果日志和异常日志各自复制一份相同判断。
    void writeOperationLog(std::shared_ptr<DatabaseManagerInterface> dbManager,
                           const std::string &module,
                           const std::string &action,
                           const std::string &result,
                           const std::string &summary,
                           const std::string &details,
                           const std::string &source,
                           std::optional<int> userId)
    {
        if (userId.has_value() && userId.value() > 0)
        {
            OperationLogger::logUserOperation(dbManager, userId.value(), module, action, result, summary, details, source);
            return;
        }

        OperationLogger::logSystemOperation(dbManager, module, action, result, summary, details, source);
    }
}


/// @brief 记录系统操作日志，不包含具体用户信息，适合记录定时任务、匿名请求和鉴权失败等事件。
/// @param dbManager 数据库管理器
/// @param module 模块名称
/// @param action 操作名称
/// @param result 操作结果
/// @param summary 操作摘要
/// @param details 操作详情
/// @param source 操作来源
void OperationLogger::logSystemOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &module, const std::string &action, const std::string &result, const std::string &summary, const std::string &details, const std::string &source)
{
    try
    {
        if (!dbManager || !dbManager->getSession())
        {
            return;
        }

        mysqlx::Session *session = dbManager->getSession();

        // 系统日志不依赖具体用户实体，适合记录匿名请求、鉴权失败和内部异常。
        session->sql("INSERT INTO system_operations "
                     "(category, operator, module, action, result, summary, details, source) "
                     "VALUES ('系统类', '系统', ?, ?, ?, ?, ?, ?)")
            .bind(module, action, result, fallbackSummary(summary, action), details, fallbackSource(source))
            .execute();
        std::cout << "系统操作日志记录成功" << std::endl;
        AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
    }
    catch (const std::exception &e)
    {
        std::cout << "日志记录失败: " << e.what() << std::endl;
    }
}


/// @brief 兼容定时任务等调用方没有足够上下文信息的场景，提供简化接口直接记录模块和详情。
/// @param dbManager 数据库管理器
/// @param module 模块名称
/// @param details 操作详情
void OperationLogger::logSystemOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &module, const std::string &details)
{
    // 兼容既有定时任务调用，避免连带修改旧的系统巡检逻辑。
    logSystemOperation(dbManager, module, "定时任务", "成功", module + "定时任务", details, "scheduled-task");
}


/// @brief 记录用户操作日志，包含用户 ID 和角色信息，适合记录需要关联具体用户的操作事件。
/// @param dbManager 数据库管理器
/// @param userId 用户 ID
/// @param module 模块名称
/// @param action 操作名称
/// @param result 操作结果
/// @param summary 操作摘要
/// @param details 操作详情
/// @param source 操作来源
void OperationLogger::logUserOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, int userId, const std::string &module, const std::string &action, const std::string &result, const std::string &summary, const std::string &details, const std::string &source)
{
    try
    {
        if (!dbManager || !dbManager->getSession() || userId <= 0)
        {
            return;
        }

        mysqlx::Session *session = dbManager->getSession();

        try
        {
            session->sql("INSERT INTO user_operations "
                         "(user_id, category, user_role, operator, module, action, result, summary, details, source) "
                         "SELECT u.id, '用户类', t.type, u.name, ?, ?, ?, ?, ?, ? "
                         "FROM users AS u "
                         "LEFT JOIN types AS t ON u.type_id = t.id "
                         "WHERE u.id = ?")
                .bind(module, action, result, fallbackSummary(summary, action), details, fallbackSource(source), userId)
                .execute();
        }
        catch (const std::exception &roleError)
        {
            session->sql("INSERT INTO user_operations "
                         "(user_id, category, user_role, operator, module, action, result, summary, details, source) "
                         "SELECT u.id, '用户类', NULL, u.name, ?, ?, ?, ?, ?, ? "
                         "FROM users AS u "
                         "WHERE u.id = ?")
                .bind(module, action, result, fallbackSummary(summary, action), details, fallbackSource(source), userId)
                .execute();
            std::cout << "用户操作日志记录成功，角色字段已降级为空: " << roleError.what() << std::endl;
            AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
            return;
        }

        std::cout << "用户操作日志记录成功" << std::endl;
        AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
    }
    catch (const std::exception &e)
    {
        std::cout << "日志记录失败: " << e.what() << std::endl;
    }
}


// 将响应解析与日志落库组合为统一收口，适合在路由返回前直接调用。
void OperationLogger::FinalizeResponseWithOperationLog(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                                       const crow::request &req,
                                                       const crow::response &res,
                                                       const std::string &module,
                                                       const std::string &action,
                                                       std::optional<int> userId)
{
    // 这个函数保留为“响应收口适配层”：把 HTTP 响应转换成统一操作日志，而不是让路由自己解析响应体。
    if (req.method == crow::HTTPMethod::Options)
    {
        return;
    }

    nlohmann::json responseBody = parseResponseBody(res);
    const std::string result = getOperationResult(res);
    const std::string summary = buildOperationSummary(module, action, res, responseBody);
    const std::string details = buildOperationDetails(req, res, responseBody);
    const std::string source = getMethodName(req.method) + " " + req.url;

    writeOperationLog(dbManager, module, action, result, summary, details, source, userId);
}


/// @brief 统一封装“是否记录成功日志”和最终 res.end()，避免各路由复制同一段模板代码。
/// @param dbManager 数据库管理器
/// @param req 请求对象
/// @param res 响应对象
/// @param module 模块名称
/// @param action 操作名称
/// @param userId 用户ID（如果有的话）
/// @param logSuccess 记录是否成功
void OperationLogger::FinishLoggedRoute(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                        const crow::request &req,
                                        crow::response &res,
                                        const std::string &module,
                                        const std::string &action,
                                        std::optional<int> userId,
                                        bool logSuccess)
{
    if (logSuccess || res.code >= 400)
    {
        FinalizeResponseWithOperationLog(dbManager, req, res, module, action, userId);
    }

    res.end();
}


/// @brief 记录鉴权失败场景的安全日志，和普通结果日志区分开，避免日志里充斥大量权限不足的失败记录，影响审计和排障效率。
/// @param dbManager 数据库管理器
/// @param req 请求对象
/// @param res 响应对象
/// @param module 模块名称
/// @param action 操作名称
/// @param userId 用户ID
void OperationLogger::LogAuthorizationFailure(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                              const crow::request &req,
                                              const crow::response &res,
                                              const std::string &module,
                                              const std::string &action,
                                              std::optional<int> userId)
{
    const nlohmann::json responseBody = parseResponseBody(res);
    std::optional<int> actorUserId = userId.has_value() && userId.value() > 0
                                         ? userId
                                         : getUserIdFromRequestToken(req);
    std::string message = getJsonStringField(responseBody, "message");

    if (message.empty() && responseBody.contains("error") && responseBody["error"].is_object())
    {
        message = getJsonStringField(responseBody["error"], "details");
    }

    const bool isPermissionFailure =
        message.find("无权限") != std::string::npos ||
        message.find("denied") != std::string::npos ||
        message.find("forbidden") != std::string::npos;

    const std::string errorType = isPermissionFailure ? "permission_denied" : "authentication_failed";
    const std::string stage = isPermissionFailure ? "authorization" : "authentication";

    nlohmann::json details = {
        {"method", getMethodName(req.method)},
        {"path", req.url},
        {"httpStatus", res.code},
        {"clientIp", RequestUtils::getClientIp(req)},
        {"stage", stage},
        {"errorType", errorType},
        {"attemptedUserId", actorUserId.has_value() ? nlohmann::json(actorUserId.value()) : nlohmann::json(nullptr)},
        {"reason", message.empty() ? "authorization failed" : message}};

    const std::string summary = module + "-" + action + ": " + errorType;
    const std::string source = getMethodName(req.method) + " " + req.url;

    writeOperationLog(dbManager, module, action, "失败", summary, details.dump(), source, actorUserId);
}

/// @brief 鉴权失败场景只记录安全日志，不再重复记录普通失败结果日志。
/// @param dbManager 数据库管理器
/// @param req 请求对象
/// @param res 响应对象
/// @param module 模块名称
/// @param action 操作名称
/// @param userId 用户ID（如果有的话）
void OperationLogger::FinishAuthorizationFailure(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                                 const crow::request &req,
                                                 crow::response &res,
                                                 const std::string &module,
                                                 const std::string &action,
                                                 std::optional<int> userId)
{
    LogAuthorizationFailure(dbManager, req, res, module, action, userId);
    res.end();
}

/// @brief 记录异常操作日志，适合在 catch 块里调用，捕获并记录路由、控制器和数据库等不同阶段的异常事件。
/// @param dbManager 数据库管理器
/// @param req 请求对象
/// @param module 模块名称
/// @param action 操作名称
/// @param exceptionMessage 异常消息
/// @param userId 用户ID
/// @param stage 阶段信息（如 route/controller/database），如果不传则根据异常文本自动推断，方便后续检索
/// @param errorType 错误类型（如 route_exception/database_error/validation_error），如果不传则根据阶段自动推断，保持同类异常聚合
void OperationLogger::LogExceptionOperation(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                            const crow::request &req,
                                            const std::string &module,
                                            const std::string &action,
                                            const std::string &exceptionMessage,
                                            std::optional<int> userId,
                                            const std::string &stage,
                                            const std::string &errorType)
{
    std::string resolvedStage = stage;
    std::string resolvedErrorType = errorType;

    // 未显式指定阶段时，根据异常文本做轻量推断，方便后续检索。
    if (resolvedStage.empty())
    {
        if (exceptionMessage == "route exception")
        {
            resolvedStage = "route";
        }
        else if (exceptionMessage.find("database") != std::string::npos ||
                 exceptionMessage.find("Database") != std::string::npos)
        {
            resolvedStage = "database";
        }
        else
        {
            resolvedStage = "controller";
        }
    }

    // 错误类型与阶段保持一致，尽量让同类异常聚合到一起。
    if (resolvedErrorType.empty())
    {
        if (resolvedStage == "route")
        {
            resolvedErrorType = "route_exception";
        }
        else if (resolvedStage == "database")
        {
            resolvedErrorType = "database_error";
        }
        else
        {
            resolvedErrorType = "exception";
        }
    }

    nlohmann::json details = {
        {"method", getMethodName(req.method)},
        {"path", req.url},
        {"clientIp", RequestUtils::getClientIp(req)},
        {"stage", resolvedStage},
        {"errorType", resolvedErrorType},
        {"exception", exceptionMessage}};

    const std::string summary = module + "-" + action + ": " + resolvedErrorType;
    const std::string source = getMethodName(req.method) + " " + req.url;

    // 异常日志与结果日志走同一套分流规则，避免后续两边行为漂移。
    writeOperationLog(dbManager, module, action, "失败", summary, details.dump(), source, userId);
}
