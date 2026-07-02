#include "UserRoleCache.h"

#include "../RedisClient.h"

namespace UserRoleCache
{
    std::string userRoleKey(int userId)
    {
        return "user:role:" + std::to_string(userId);
    }

    int userRoleTtlSeconds()
    {
        return 300;
    }

    std::optional<std::string> readCache(int userId)
    {
        if (userId <= 0)
        {
            return std::nullopt;
        }

        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return std::nullopt;
        }

        return redis.get(userRoleKey(userId));
    }

    bool writeCache(int userId, const std::string &roleName)
    {
        if (userId <= 0 || roleName.empty())
        {
            return false;
        }

        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return false;
        }

        return redis.setEx(userRoleKey(userId), userRoleTtlSeconds(), roleName);
    }

    void invalidate(int userId)
    {
        if (userId <= 0)
        {
            return;
        }

        RedisClient &redis = RedisClient::instance();
        if (redis.enabled())
        {
            redis.del(userRoleKey(userId));
        }
    }
}
