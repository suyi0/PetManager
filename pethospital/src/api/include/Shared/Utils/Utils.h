#pragma once
#include <crow.h>
#include <nlohmann/json.hpp>

// 工具函数声明
bool parseJsonBody(const crow::request &req, crow::response &res, nlohmann::json &request_body);
std::string getCreateTime();

// 判断响应结果函数
void ProcessHandlerResponse(const crow::request &req, crow::response &res, crow::response &handlerResponse);


// CORS中间件类
class CorsMiddleware
{
public:
    struct context
    {
    };

    // 使用默认构造函数
    CorsMiddleware() = default;

    void before_handle(crow::request &req, crow::response &res, context &ctx);
    void after_handle(crow::request &req, crow::response &res, context &ctx);
};

// 限制请求速率中间件类
class RateLimitMiddleware
{
public:
    struct context
    {
        bool rate_limited = false;
    };

    // 使用默认构造函数
    RateLimitMiddleware() {} // 使用默认值

    // 设置限制参数
    static void initialize(int max_requests = 50, int window_seconds = 60) {
        RateLimitMiddleware::max_requests_global_ = max_requests;
        RateLimitMiddleware::window_seconds_global_ = window_seconds;
    }

    void before_handle(crow::request &req, crow::response &res, context &ctx);
    void after_handle(crow::request &req, crow::response &res, context &ctx);

private:
    // 获取客户端IP地址
    std::string getClientIP(const crow::request& req);

    static int max_requests_global_;
    static int window_seconds_global_;

    // 存储每个IP的请求时间点的列表
    std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> request_counts_;
    // 互斥锁保护共享数据
    std::mutex mutex_;
};

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

    static crow::response unauthorized(const crow::request &req, const std::string &message = "Unauthorized")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(401, error_data.dump());
        return res;
    }

    static crow::response notFound(const crow::request &req, const std::string &message = "Not Found")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(404, error_data.dump());
        return res;
    }

    static crow::response validation(const crow::request &req, const std::string &message = "Validation failed")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(422, error_data.dump());
        return res;
    }

    static crow::response system_error(const crow::request &req, const std::string &message = "Internal Server Error")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(500, error_data.dump());
        return res;
    }

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


using CrowApp = crow::Crow<CorsMiddleware, RateLimitMiddleware>;