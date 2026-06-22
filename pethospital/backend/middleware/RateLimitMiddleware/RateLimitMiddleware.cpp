#include "rateLimitMiddleware.h"
// 限制速率RateLimit中间件
// 静态成员定义
int RateLimitMiddleware::max_requests_global_ = 50;
int RateLimitMiddleware::window_seconds_global_ = 60;

bool RateLimitMiddleware::shouldRateLimitPath(const std::string &path, const std::string &method)
{
    if (method == "OPTIONS")
    {
        return false;
    }

    return path == "/api" || path.rfind("/api/", 0) == 0 ||
           path == "/realtime" || path.rfind("/realtime/", 0) == 0;
}

void RateLimitMiddleware::before_handle(crow::request &req, crow::response &res, context &ctx)
{
    if (!shouldRateLimitPath(req.url, crow::method_name(req.method)))
    {
        return;
    }

    // 获取客户端IP
    std::string client_ip = getClientIP(req);

    // 锁定互斥锁
    std::lock_guard<std::mutex> lock(mutex_);

    // 获取当前时间
    auto now = std::chrono::steady_clock::now();
    // 查找或创建该IP的请求记录
    auto it = request_counts_.find(client_ip);

    if (it != request_counts_.end()) // 找到已有记录
    {
        // 清理过期的请求记录
        auto &records = it->second;                                         // 获取该IP的请求记录
        auto cutoff = now - std::chrono::seconds(window_seconds_global_);   // 计算时间窗口的截止时间

        // 移除过期记录
        //std::remove_if将满足条件（record < cutoff）的元素移到容器末尾，并返回指向新末尾的迭代器
        //records.erase使用该迭代器删除从新末尾到原末尾的所有元素，实现了移除容器中所有小于cutoff值的记录的功能
        //erase(remove_if_result, records.end()) 就能一次性删除从新末尾到原始末尾之间的所有元素。
        records.erase(
            std::remove_if(records.begin(), records.end(),
                           [&cutoff](const auto &record)
                           {
                               return record < cutoff;
                           }),
            records.end());

        // 检查是否超过限制
        if (records.size() >= max_requests_global_)
        {
            res.code = 429; // Too Many Requests
            res.set_header("Content-Type", "application/json");

            // 添加CORS头以确保跨域请求正常工作
            std::string origin = req.get_header_value("Origin");
            if (origin == "http://localhost:8080" || origin == "http://127.0.0.1:8080")
            {
                res.set_header("Access-Control-Allow-Origin", origin);
            }
            else
            {
                res.set_header("Access-Control-Allow-Origin", origin.empty() ? "*" : origin);
            }
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
            res.set_header("Access-Control-Allow-Credentials", "true");

            // 构建错误响应体
            nlohmann::json error_response;
            error_response["error"] = "Rate limit exceeded";
            error_response["retry_after"] = window_seconds_global_;

            res.body = error_response.dump();
            res.end();
            ctx.rate_limited = true;
            return;
        }
    }
    else
    {
        // 初始化用户请求记录
        request_counts_[client_ip] = std::vector<std::chrono::steady_clock::time_point>();
    }
}

void RateLimitMiddleware::after_handle(crow::request &req, crow::response &res, context &ctx)
{
    if (!shouldRateLimitPath(req.url, crow::method_name(req.method)))
    {
        return;
    }

    // 只有当请求未被限流时才记录(判断rate_limited标志)
    if (ctx.rate_limited)
    {
        return;
    }

    // 获取客户端IP
    std::string client_ip = getClientIP(req);

    // 锁定互斥锁
    std::lock_guard<std::mutex> lock(mutex_);

    // 添加当前请求的时间戳到记录中
    auto now = std::chrono::steady_clock::now();
    // 查找该IP的请求记录
    auto it = request_counts_.find(client_ip);

    if (it != request_counts_.end())    // 找到已有记录（将当前时间添加到记录中）
    {
        it->second.push_back(now);
    }
    else                                // 没有该IP的记录，则创建新的记录
    {
        // 理论上不应该发生，但为了安全起见
        std::vector<std::chrono::steady_clock::time_point> records;
        records.push_back(now);
        request_counts_[client_ip] = records;
    }
}

std::string RateLimitMiddleware::getClientIP(const crow::request &req)
{
    std::string client_ip = req.get_header_value("X-Forwarded-For");    // 获取X-Forwarded-For头
    if (!client_ip.empty())
    {
        // X-Forwarded-For 可能包含多个IP地址，取第一个
        size_t pos = client_ip.find(',');
        //std::string::npos 是C++标准库中定义的一个常量，表示字符串查找操作未找到匹配项时返回的值
        if (pos != std::string::npos)                   // 找到逗号，取第一个IP
        {
            client_ip = client_ip.substr(0, pos);
        }
        return client_ip;
    }

    client_ip = req.get_header_value("X-Real-IP");      // 获取X-Real-IP头
    if (!client_ip.empty())
    {
        size_t pos = client_ip.find(',');
        if(pos != std::string::npos)
        {
            client_ip = client_ip.substr(0, pos);
        }
        return client_ip;
    }

    client_ip = req.get_header_value("CF-Connecting-IP"); // CloudFlare
    if (!client_ip.empty())
    {
        size_t pos = client_ip.find(',');
        if(pos != std::string::npos)
        {
            client_ip = client_ip.substr(0, pos);
        }
        return client_ip;
    }

    client_ip = req.get_header_value("X-Original-For");     // 一些代理服务器使用的头
    if (!client_ip.empty())
    {
        size_t pos = client_ip.find(',');
        if(pos != std::string::npos)
        {
            client_ip = client_ip.substr(0, pos);
        }
        return client_ip;
    }

    client_ip = req.get_header_value("X-Cluster-Client-IP");        // 一些代理服务器使用的头
    if (!client_ip.empty())
    {
        size_t pos = client_ip.find(',');
        if(pos != std::string::npos)
        {
            client_ip = client_ip.substr(0, pos);
        }
        return client_ip;
    }

    // 如果无法获取客户端IP，使用远程端点
    return "unknown";
}
