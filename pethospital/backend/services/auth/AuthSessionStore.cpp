#include "AuthSessionStore.h"

#include "../redis/RedisClient.h"
#include "../../utils/roleTypeUtils/roleTypeUtils.h"

#include <iostream>

namespace AuthSessionStore
{
    std::string sessionVersionKey(int userId)
    {
        return "auth:session-version:" + std::to_string(userId);
    }

    std::optional<int> parseSessionVersion(const std::optional<std::string> &raw)
    {
        if (!raw.has_value())
        {
            return std::nullopt;
        }

        try
        {
            int value = std::stoi(raw.value());
            if (value < 0)
            {
                return std::nullopt;
            }
            return value;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    bool isTokenVersionCurrent(int tokenVersion, const std::optional<int> &storedVersion)
    {
        if (!storedVersion.has_value())
        {
            return true;
        }
        return tokenVersion == storedVersion.value();
    }

    int issueVersionForRole(int userId, const std::string &roleName)
    {
        if (userId <= 0 || !RoleTypeUtils::isManagementRole(roleName))
        {
            return kDefaultSessionVersion;
        }

        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return kDefaultSessionVersion;
        }

        return parseSessionVersion(redis.get(sessionVersionKey(userId))).value_or(kDefaultSessionVersion);
    }

    bool isSessionCurrent(int userId, const std::string &roleName, int tokenVersion)
    {
        if (userId <= 0)
        {
            return false;
        }

        if (!RoleTypeUtils::isManagementRole(roleName))
        {
            return true;
        }

        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return true;
        }

        std::optional<int> storedVersion = parseSessionVersion(redis.get(sessionVersionKey(userId)));
        return isTokenVersionCurrent(tokenVersion, storedVersion);
    }

    bool bumpSessionVersionForUser(int userId)
    {
        if (userId <= 0)
        {
            return false;
        }

        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return false;
        }

        // 原子自增：避免并发改密码/登出时 GET+SET 丢增量（两请求读到同值各 +1，只生效一次）。
        // INCR 在键不存在时从 0 自增为 1，与原先 default(0)+1 的语义一致。
        std::optional<long long> nextVersion = redis.incr(sessionVersionKey(userId));
        if (!nextVersion.has_value())
        {
            std::cerr << "Failed to bump auth session version for user " << userId << std::endl;
            return false;
        }

        return true;
    }
}
