#include "AuthLoginFailureStore.h"

#include <optional>

#include "../redis/RedisClient.h"

namespace AuthLoginFailureStore
{
    namespace
    {
        constexpr int kMaxFailures = 5;     // 达到此次数即锁定
        constexpr int kWindowSeconds = 900; // 失败计数窗口 + 锁定时长（15 分钟）

        std::string failKey(const std::string &clientIp, const std::string &identifier)
        {
            return "auth:login-fail:" + clientIp + ":" + identifier;
        }

        std::string lockKey(const std::string &clientIp, const std::string &identifier)
        {
            return "auth:login-lock:" + clientIp + ":" + identifier;
        }
    }

    bool isLocked(const std::string &clientIp, const std::string &identifier)
    {
        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return false;
        }
        return redis.get(lockKey(clientIp, identifier)).has_value();
    }

    void recordFailure(const std::string &clientIp, const std::string &identifier)
    {
        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return;
        }

        std::optional<long long> fails = redis.incr(failKey(clientIp, identifier));
        if (!fails.has_value())
        {
            return;
        }
        if (fails.value() == 1)
        {
            redis.expire(failKey(clientIp, identifier), kWindowSeconds); // 首次失败开窗
        }
        if (fails.value() >= kMaxFailures)
        {
            redis.setEx(lockKey(clientIp, identifier), kWindowSeconds, "1");
        }
    }

    void clearOnSuccess(const std::string &clientIp, const std::string &identifier)
    {
        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return;
        }
        redis.del(failKey(clientIp, identifier));
        redis.del(lockKey(clientIp, identifier));
    }
}
