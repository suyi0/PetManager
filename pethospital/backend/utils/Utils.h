#pragma once
#include <crow.h>
#include <ctime>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <algorithm>
#include <stdexcept>
#include <mysqlx/xdevapi.h>
#include "../middleware/corsMiddleware/corsMiddleware.h"
#include "../middleware/rateLimitMiddleware/rateLimitMiddleware.h"
#include "../database/DatabaseManager.h"
#include "../controllers/auth/encrypt/encrypt.h"
#include "../controllers/auth/jwtUtils/jwtUtils.h"
#include "../database/DatabaseManager.h"
#include "ResponseCodes.h"

// 工具函数声明
std::tm safeLocalTime(std::time_t time_value);
std::string getCreateTime();
std::string clean_string(const std::string &input);
std::string format_date(const std::tm &tm);
std::string normalizeDate(const std::string &date_str);
std::string getRequestString(const nlohmann::json &request_body, const std::string &key, const std::string &defaultValue = "");
std::string getRequestStringWithFallback(const nlohmann::json &request_body, const std::string &primaryKey, const std::string &fallbackKey, const std::string &DBValue = "");
double getRequestDouble(const nlohmann::json &request_body, const std::string &key, double defaultValue = 0.0);
double getRequestDoubleWithFallback(const nlohmann::json &request_body, const std::string &primaryKey, const std::string &fallbackKey, double defaultValue = 0.0);
bool constantTimeEquals(const std::string &left, const std::string &right);
void rollbackTransactionQuietly(mysqlx::Session &session);

std::string formatDateTime(const boost::posix_time::ptime &pt);
std::string formatDateOnly(const boost::posix_time::ptime &pt); // 只提取日期部分
std::string formatTimeOnly(const boost::posix_time::ptime &pt); // 只提取时间部分

// 环境变量相关函数
bool loadEnvironmentFile(const std::string &envFilePath);                        // 加载环境变量文件
std::string getEnvVar(const std::string &name, const std::string &defaultValue); // 获取环境变量，带默认值
void initializeEnvironment();                                                    // 在main函数开始时调用

// 判断响应结果函数
void ProcessHandlerResponse(const crow::request &req, crow::response &res, crow::response &handlerResponse);

// 响应帮助类
class ResponseHelper
{
public:
    // 统一成功响应：
    // 返回固定结构 { success, code, message, data, error }，
    // 其中 code=0 表示业务成功。
    static crow::response success(const crow::request &req, const nlohmann::json &data)
    {
        return buildResponse(200, ResponseCode::Success, "success", data, nullptr);
    }

    // 统一创建成功响应：
    // 适用于新增资源等场景，HTTP 状态码为 201。
    static crow::response created(const crow::request &req, const nlohmann::json &data)
    {
        return buildResponse(201, ResponseCode::Success, "created", data, nullptr);
    }

    // 通用失败响应：
    // 用于需要明确指定 HTTP 状态码、业务码、错误类型和细节的场景。
    static crow::response fail(
        const crow::request &req,
        int httpStatus,
        int businessCode,
        const std::string &message,
        const std::string &errorType,
        const std::string &details = "")
    {
        nlohmann::json error = {
            {"type", errorType},
            {"details", details.empty() ? message : details},
        };

        return buildResponse(httpStatus, businessCode, message, nullptr, error);
    }

    // 通用客户端错误响应：
    // 默认用于参数错误或普通业务失败。
    static crow::response error(const crow::request &req, const std::string &message)
    {
        return fail(req, 400, ResponseCode::BadRequest, message, ResponseErrorType::BadRequest, message);
    }

    // 未授权响应：
    // 适用于 token 缺失、无效、过期等场景。
    static crow::response unauthorized(const crow::request &req, const std::string &message = "Unauthorized")
    {
        return fail(req, 401, ResponseCode::AuthError, message, ResponseErrorType::AuthError, message);
    }

    // 未找到响应：
    // 适用于用户、订单、预约记录不存在等场景。
    static crow::response notFound(const crow::request &req, const std::string &message = "Not Found")
    {
        return fail(req, 404, ResponseCode::NotFound, message, ResponseErrorType::NotFound, message);
    }

    // 参数校验失败响应：
    // 适用于字段缺失、格式不正确、输入非法等场景。
    static crow::response validation(const crow::request &req, const std::string &message = "Validation failed")
    {
        return fail(req, 400, ResponseCode::ValidationError, message, ResponseErrorType::ValidationError, message);
    }

    // 系统内部错误响应：
    // 适用于数据库不可用、内部异常、未知错误等场景。
    static crow::response system_error(const crow::request &req, const std::string &message = "Internal Server Error")
    {
        return fail(req, 500, ResponseCode::InternalServerError, message, ResponseErrorType::SystemError, message);
    }

    // 服务不可用响应：
    // 适用于依赖服务故障、临时不可用或降级场景。
    static crow::response unavailable(const crow::request &req, const std::string &message = "Service Unavailable")
    {
        return fail(req, 503, ResponseCode::ServiceUnavailable, message, ResponseErrorType::ServiceUnavailable, message);
    }

    // 数据库错误响应：
    // 适用于 SQL 执行失败、数据库查询异常、连接后续操作失败等场景。
    static crow::response database_error(
        const crow::request &req,
        const std::string &message = "Database error",
        const std::string &details = "")
    {
        return fail(req, 500, ResponseCode::DatabaseError, message, ResponseErrorType::DatabaseError, details);
    }

    // 权限不足响应：
    // 适用于当前用户无权访问目标资源或无权执行当前操作的场景。
    static crow::response permission_denied(
        const crow::request &req,
        const std::string &message = "Permission denied",
        const std::string &details = "")
    {
        return fail(req, 403, ResponseCode::PermissionDenied, message, ResponseErrorType::PermissionError, details);
    }

    // 验证码失败响应：
    // 适用于短信验证码、邮箱验证码校验失败等认证流程场景。
    static crow::response verification_failed(
        const crow::request &req,
        const std::string &message = "Invalid verification code",
        const std::string &details = "")
    {
        return fail(req, 401, ResponseCode::VerificationCodeInvalid, message, ResponseErrorType::AuthError, details);
    }

    // 操作失败响应：
    // 适用于带有具体错误细节的通用服务端失败场景。
    static crow::response operation_failed(
        const crow::request &req,
        const std::string &message = "Operation failed",
        const std::string &details = "")
    {
        return fail(req, 500, ResponseCode::SystemError, message, ResponseErrorType::SystemError, details);
    }

private:
    // 构造最终响应对象，并补齐统一的协议字段。
    static crow::response buildResponse(
        int httpStatus,
        int businessCode,
        const std::string &message,
        const nlohmann::json &data,
        const nlohmann::json &error)
    {
        const bool ok = httpStatus >= 200 && httpStatus < 300;

        nlohmann::json payload = {
            {"success", ok},
            {"code", businessCode},
            {"message", message},
            {"data", data.is_null() ? nlohmann::json(nullptr) : data},
            {"error", error.is_null() ? nlohmann::json(nullptr) : error},
        };

        // 用 replace 容错：DB 里若存在非法 UTF-8 字节（如 0x0F），用替换符代替而非抛出
        // json type_error.316，避免整个响应序列化失败导致接口 500、前端整页报错。
        crow::response res(
            httpStatus,
            payload.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
        res.set_header("Content-Type", "application/json");
        return res;
    }
};

// 基类
class BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit BaseHandler(std::shared_ptr<DatabaseManagerInterface> db) : dbManager(db) {}

    // 公共 JSON 解析方法：
    // 当请求体不是合法 JSON 时，直接返回统一的参数校验失败响应。
    std::optional<nlohmann::json> parseJson(const crow::request &req, crow::response &res)
    {
        try
        {
            return nlohmann::json::parse(req.body);
        }
        catch (...)
        {
            res = ResponseHelper::fail(
                req,
                400,
                ResponseCode::ValidationError,
                "Invalid JSON",
                ResponseErrorType::ValidationError,
                "Request body is not valid JSON");
            return std::nullopt;
        }
    }

    // 检查数据库连接
    bool checkDbConnection()
    {
        if (!dbManager || !dbManager->getSession() || !dbManager->getSchema())
        {
            return false;
        }
        return true;
    }

    // 一键验证请求：
    // 同时完成 JSON 解析和数据库连接检查，失败时返回统一错误响应。
    std::optional<nlohmann::json> validateRequest(const crow::request &req, crow::response &res)
    {
        auto json_opt = parseJson(req, res);
        if (!json_opt)
            return std::nullopt;

        if (!checkDbConnection())
        {
            res = ResponseHelper::system_error(req);
            return std::nullopt;
        }

        return json_opt;
    }
};

using CrowApp = crow::Crow<CorsMiddleware, RateLimitMiddleware>;
