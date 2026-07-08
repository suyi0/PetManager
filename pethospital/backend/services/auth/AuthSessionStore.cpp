#include "AuthSessionStore.h"

#include "../redis/RedisClient.h"

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

    // 会话版本对全员生效（原先只对管理角色）：动态 RBAC 下派职位/改权限随时可能发生在任何用户身上，
    // 版本 bump 是"降权即时生效"的载体，不能按 token 里的旧角色名决定是否检查。
    int issueVersion(int userId)
    {
        if (userId <= 0)
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

    bool isSessionCurrent(int userId, int tokenVersion)
    {
        if (userId <= 0)
        {
            return false;
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
