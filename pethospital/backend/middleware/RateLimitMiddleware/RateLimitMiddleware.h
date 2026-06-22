#pragma once
#include <crow.h>
#include <nlohmann/json.hpp>

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

    static bool shouldRateLimitPath(const std::string &path, const std::string &method);

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
