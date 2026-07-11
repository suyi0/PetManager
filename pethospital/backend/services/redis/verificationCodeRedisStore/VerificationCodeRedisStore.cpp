#include "VerificationCodeRedisStore.h"

#include "../RedisClient.h"

namespace VerificationCodeRedisStore
{
    namespace
    {
        // 验证码存储的key
        std::string codeKey(const std::string &identifier)
        {
            return "verify:code:" + identifier;
        }
    }

    bool store(const std::string &identifier, const std::string &code, int ttlSeconds)
    {
        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return false;
        }
        return redis.setEx(codeKey(identifier), ttlSeconds, code);  // 存储验证码
    }

    std::optional<std::string> get(const std::string &identifier)
    {
        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return std::nullopt;
        }
        return redis.get(codeKey(identifier));
    }

    bool del(const std::string &identifier)
    {
        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return false;
        }
        return redis.del(codeKey(identifier));
    }
}
