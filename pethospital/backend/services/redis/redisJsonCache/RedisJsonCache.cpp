#include "RedisJsonCache.h"

#include <optional>
#include <string>

#include "../RedisClient.h"

namespace RedisJsonCache
{
    nlohmann::json cached(const std::string &cacheKey, int ttlSeconds,
                          const std::function<nlohmann::json()> &compute)
    {
        RedisClient &redis = RedisClient::instance();
        if (redis.enabled())
        {
            std::optional<std::string> hit = redis.get(cacheKey);
            if (hit.has_value())
            {
                try
                {
                    return nlohmann::json::parse(hit.value());
                }
                catch (...)
                {
                    // 缓存值损坏：当作未命中，继续查库覆盖。
                }
            }
            nlohmann::json value = compute();
            redis.setEx(cacheKey, ttlSeconds, value.dump());
            return value;
        }
        return compute();
    }
}
