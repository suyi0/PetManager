#include "DoctorListCache.h"

#include "../RedisClient.h"

namespace DoctorListCache
{
    std::string doctorCacheVersionKey()
    {
        return "doctor:list:cache-version:v1";
    }

    long long defaultDoctorCacheVersion()
    {
        return 0;
    }

    std::string doctorListKey(const std::string &date, long long version)
    {
        return "doctor:list:v1:" + date + ":" + std::to_string(version);
    }

    int doctorListTtlSeconds()
    {
        return 30;
    }

    nlohmann::json cachedDoctorList(const std::string &date,
                                    const std::function<nlohmann::json()> &compute)
    {
        std::optional<long long> version = currentCacheVersion();
        if (version.has_value())
        {
            std::optional<nlohmann::json> cached = readDoctorListCache(date, version.value());
            if (cached.has_value())
            {
                return cached.value();
            }
        }

        nlohmann::json value = compute();
        if (version.has_value())
        {
            writeDoctorListCache(date, version.value(), value);
        }
        return value;
    }

    namespace
    {
        std::optional<nlohmann::json> parseDoctorListCache(const std::optional<std::string> &raw)
        {
            if (!raw.has_value())
            {
                return std::nullopt;
            }

            try
            {
                nlohmann::json parsed = nlohmann::json::parse(raw.value());
                if (!parsed.is_array())
                {
                    return std::nullopt;
                }
                return parsed;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        std::optional<long long> readCacheVersion(RedisClient &redis)
        {
            std::optional<std::string> raw = redis.get(doctorCacheVersionKey());
            if (!raw.has_value())
            {
                return defaultDoctorCacheVersion();
            }

            try
            {
                long long version = std::stoll(raw.value());
                if (version < 0)
                {
                    return defaultDoctorCacheVersion();
                }
                return version;
            }
            catch (...)
            {
                return defaultDoctorCacheVersion();
            }
        }
    }

    std::optional<long long> currentCacheVersion()
    {
        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return std::nullopt;
        }
        return readCacheVersion(redis);
    }

    std::optional<nlohmann::json> readDoctorListCache(const std::string &date, long long version)
    {
        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return std::nullopt;
        }
        return parseDoctorListCache(redis.get(doctorListKey(date, version)));
    }

    bool writeDoctorListCache(const std::string &date, long long version, const nlohmann::json &doctors)
    {
        if (!doctors.is_array())
        {
            return false;
        }

        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return false;
        }

        return redis.setEx(doctorListKey(date, version), doctorListTtlSeconds(), doctors.dump());
    }

    void invalidateDoctorList()
    {
        RedisClient &redis = RedisClient::instance();
        if (redis.enabled())
        {
            redis.incr(doctorCacheVersionKey());
        }
    }
}
