#include "RedisCountCache.h"

#include <optional>
#include <string>

#include "../RedisClient.h"

namespace RedisCountCache
{
    int cached(const std::string &cacheKey, int ttlSeconds, const std::function<int()> &compute)
    {
        RedisClient &redis = RedisClient::instance();
        if (redis.enabled())
        {
            std::optional<std::string> hit = redis.get(cacheKey);
            if (hit.has_value())
            {
                try
                {
                    return std::stoi(hit.value());
                }
                catch (...)
                {
                    // 缓存值异常：当作未命中，继续查库覆盖。
                }
            }
            int value = compute();
            redis.setEx(cacheKey, ttlSeconds, std::to_string(value));
            return value;
        }
        return compute();
    }
}
