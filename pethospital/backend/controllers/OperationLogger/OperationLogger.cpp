#include "OperationLogger.h"

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

// 这里复用常见代理头获取真实客户端 IP，避免日志里只看到网关地址。
std::string getRequestClientIp(const crow::request &req)
{
    static const std::array<const char *, 5> headers = {
        "X-Forwarded-For",
        "X-Real-IP",
        "CF-Connecting-IP",
        "X-Original-For",
        "X-Cluster-Client-IP"};

    for (const char *header : headers)
    {
        std::string value = req.get_header_value(header);
        if (!value.empty())
        {
            const size_t pos = value.find(',');
            return pos == std::string::npos ? value : value.substr(0, pos);
        }
    }

    return "unknown";
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
        {"clientIp", getRequestClientIp(req)},
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

void OperationLogger::logSystemOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &module, const std::string &action, const std::string &result, const std::string &summary, const std::string &details, const std::string &source)
{
    try {
        if (!dbManager || !dbManager->getSession()) {
            return;
        }

        mysqlx::Session *session = dbManager->getSession();

        // 系统日志不依赖具体用户实体，适合记录匿名请求、鉴权失败和内部异常。
        session->sql("INSERT INTO system_operations "
                     "(category, operator, module, action, result, summary, details, source) "
                     "VALUES ('系统类', '系统', ?, ?, ?, ?, ?, ?)")
                .bind(module, action, result, fallbackSummary(summary, action), details, fallbackSource(source))
                .execute();
        std::cout <<  "系统操作日志记录成功" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "日志记录失败: " << e.what() << std::endl;
    }
}

void OperationLogger::logSystemOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &module, const std::string &details)
{
    // 兼容既有定时任务调用，避免连带修改旧的系统巡检逻辑。
    logSystemOperation(dbManager, module, "定时任务", "成功", module + "定时任务", details, "scheduled-task");
}

void OperationLogger::logUserOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, int userId, const std::string &module, const std::string &action, const std::string &result, const std::string &summary, const std::string &details, const std::string &source)
{
    try {
        if (!dbManager || !dbManager->getSession() || userId <= 0) {
            return;
        }

        mysqlx::Session *session = dbManager->getSession();

        session->sql("INSERT INTO user_operations "
                     "(user_id, category, user_role, operator, module, action, result, summary, details, source) "
                     "SELECT u.id, '用户类', t.type, u.name, ?, ?, ?, ?, ?, ? "
                     "FROM users AS u "
                     "LEFT JOIN types AS t ON u.type_id = t.id "
                     "WHERE u.id = ?")
                .bind(module, action, result, fallbackSummary(summary, action), details, fallbackSource(source), userId)
                .execute();
        std::cout << "用户操作日志记录成功" << std::endl;
    } catch (const std::exception& e)
    {
        std::cout << "日志记录失败: " << e.what() << std::endl;
    }
}

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
        {"clientIp", getRequestClientIp(req)},
        {"stage", stage},
        {"errorType", errorType},
        {"attemptedUserId", actorUserId.has_value() ? nlohmann::json(actorUserId.value()) : nlohmann::json(nullptr)},
        {"reason", message.empty() ? "authorization failed" : message}};

    const std::string summary = module + "-" + action + ": " + errorType;
    const std::string source = getMethodName(req.method) + " " + req.url;

    writeOperationLog(dbManager, module, action, "失败", summary, details.dump(), source, actorUserId);
}

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
        {"clientIp", getRequestClientIp(req)},
        {"stage", resolvedStage},
        {"errorType", resolvedErrorType},
        {"exception", exceptionMessage}};

    const std::string summary = module + "-" + action + ": " + resolvedErrorType;
    const std::string source = getMethodName(req.method) + " " + req.url;

    // 异常日志与结果日志走同一套分流规则，避免后续两边行为漂移。
    writeOperationLog(dbManager, module, action, "失败", summary, details.dump(), source, userId);
}
