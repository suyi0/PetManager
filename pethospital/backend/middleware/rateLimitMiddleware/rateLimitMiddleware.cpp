#include "rateLimitMiddleware.h"
#include "../../services/redis/RedisClient.h"
#include "../../utils/requestUtils/RequestUtils.h"
#include "../../utils/corsUtils/CorsUtils.h"
#include <atomic>
#include <chrono>
#include <string>

int RateLimitMiddleware::max_requests_global_ = 50;
int RateLimitMiddleware::window_seconds_global_ = 60;

namespace
{
    // 限流键前缀；按客户端 IP 分桶。
    inline std::string rateLimitKey(const std::string &clientIp)
    {
        return "ratelimit:" + clientIp;
    }

    // 当前时间（毫秒），作为滑动窗口里 ZSET 成员的 score。
    inline long long nowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    // 生成窗口内唯一成员，避免同一毫秒多次请求 score 相同被去重。
    inline std::string uniqueMember(long long ms)
    {
        static std::atomic<uint64_t> seq{0};
        return std::to_string(ms) + ":" + std::to_string(seq.fetch_add(1));
    }
}

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
    std::string client_ip = RequestUtils::getClientIp(req);

    // 判定是否超限：优先用 Redis 滑动窗口（多实例共享计数），
    // Redis 不可用时回退到原有的进程内内存窗口。
    bool over_limit = false;
    bool decided_by_redis = false;

    // Redis 优先：单条 Lua 原子完成"检查+记录"，消除 check 与 record 分离导致的并发穿透。
    if (RedisClient::instance().enabled())
    {
        long long now_ms = nowMs();
        long long cutoff_ms = now_ms - static_cast<long long>(window_seconds_global_) * 1000;

        std::optional<RedisClient::WindowHit> hit = RedisClient::instance().zWindowHit(
            rateLimitKey(client_ip), cutoff_ms, now_ms, uniqueMember(now_ms),
            window_seconds_global_ + 1, max_requests_global_);
        if (hit.has_value())
        {
            decided_by_redis = true;
            ctx.counted_by_redis = true; // 本次命中已在此原子记录，after_handle 不再重复
            over_limit = !hit->admitted;
        }
        // hit 为空表示 Redis 临时不可用 → 落到本地内存判定。
    }

    // 本地内存兜底：清理过期记录后按窗口内数量判定。
    if (!decided_by_redis)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        auto it = request_counts_.find(client_ip);

        if (it != request_counts_.end())
        {
            auto &records = it->second;
            auto cutoff = now - std::chrono::seconds(window_seconds_global_);
            records.erase(
                std::remove_if(records.begin(), records.end(),
                               [&cutoff](const auto &record)
                               {
                                   return record < cutoff;
                               }),
                records.end());

            over_limit = (records.size() >= static_cast<size_t>(max_requests_global_));
        }
        else
        {
            // 初始化用户请求记录
            request_counts_[client_ip] = std::vector<std::chrono::steady_clock::time_point>();
        }
    }

    if (over_limit)
    {
        res.code = 429; // Too Many Requests
        res.set_header("Content-Type", "application/json");

        // 添加CORS头以确保跨域请求正常工作（与 CorsMiddleware 用同一份允许列表）。
        CorsUtils::applyCorsHeaders(res, req.get_header_value("Origin"));

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

    // Redis 路径已在 before_handle 用 zWindowHit 原子记录过本次命中，这里只剩本地内存兜底。
    if (ctx.counted_by_redis)
    {
        return;
    }

    // 获取客户端IP
    std::string client_ip = RequestUtils::getClientIp(req);

    // 本地内存兜底：添加当前请求的时间戳到记录中。
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    auto it = request_counts_.find(client_ip);

    if (it != request_counts_.end()) // 找到已有记录（将当前时间添加到记录中）
    {
        it->second.push_back(now);
    }
    else // 没有该IP的记录，则创建新的记录
    {
        std::vector<std::chrono::steady_clock::time_point> records;
        records.push_back(now);
        request_counts_[client_ip] = records;
    }
}
