#pragma once
#include <crow.h>
#include <vector>
#include <nlohmann/json.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <algorithm>
#include <mysqlx/xdevapi.h>
#include "../middleware/CorsMiddleware/CorsMiddleware.h"
#include "../middleware/RateLimitMiddleware/RateLimitMiddleware.h"
#include "../database/DatabaseManager.h"
#include "../controllers/auth/Encrypt/Encrypt.h"
#include "../controllers/auth/JwtUtils/JwtUtils.h"
#include "../database/DatabaseManager.h"

// 工具函数声明
std::tm safeLocalTime(std::time_t time_value);
std::string getCreateTime();
std::string clean_string(const std::string &input);
std::string format_date(const std::tm &tm);
std::string normalizeDate(const std::string &date_str);

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
    // 成功响应需要求对象以构建响应
    static crow::response success(const crow::request &req, const nlohmann::json &data)
    {
        crow::response res(200, data.dump());
        return res;
    }

    // 创建成功响应
    static crow::response created(const crow::request &req, const nlohmann::json &data)
    {
        crow::response res(201, data.dump());
        return res;
    }

    // 错误响应直接返回错误消息和状态码即可
    static crow::response error(const crow::request &req, const std::string &message)
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(400, error_data.dump());
        return res;
    }

    // 未授权
    static crow::response unauthorized(const crow::request &req, const std::string &message = "Unauthorized")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(401, error_data.dump());
        return res;
    }

    // 未找到
    static crow::response notFound(const crow::request &req, const std::string &message = "Not Found")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(404, error_data.dump());
        return res;
    }

    // 验证错误
    static crow::response validation(const crow::request &req, const std::string &message = "Validation failed")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(422, error_data.dump());
        return res;
    }

    // 系统错误
    static crow::response system_error(const crow::request &req, const std::string &message = "Internal Server Error")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(500, error_data.dump());
        return res;
    }

    // 服务不可用
    static crow::response unavailable(const crow::request &req, const std::string &message = "Service Unavailable")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(503, error_data.dump());
        return res;
    }

    // 自定义响应传输状态码和对象
    static crow::response custom(const crow::request &req, int status, const nlohmann::json &data)
    {
        crow::response res(status, data.dump());
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

    // 公共的 JSON 解析方法
    std::optional<nlohmann::json> parseJson(const crow::request& req, crow::response& res) {
        try {
            return nlohmann::json::parse(req.body);
        } catch (...) {
            res.code = 400;
            res.set_header("Content-Type", "application/json");
            std::string origin = req.get_header_value("Origin");
            if (!origin.empty()) {
                res.set_header("Access-Control-Allow-Origin", origin);
            }
            res.write(R"({"error": "Invalid JSON"})");
            res.end();
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

    // 一键验证：JSON + 数据库（返回 optional）
    std::optional<nlohmann::json> validateRequest(const crow::request& req, crow::response& res) {
        auto json_opt = parseJson(req, res);
        if (!json_opt) return std::nullopt;
        
        if (!checkDbConnection()) {
            res = ResponseHelper::system_error(req);
            return std::nullopt;
        }
        
        return json_opt;
    }
};

using CrowApp = crow::Crow<CorsMiddleware, RateLimitMiddleware>;
