#include "MedicineRedisCache.h"

#include "../RedisClient.h"

namespace MedicineRedisCache
{
    std::string medicineCacheVersionKey()
    {
        return "doctor:medicines:cache-version:v1";
    }

    long long defaultMedicineCacheVersion()
    {
        return 0;
    }

    std::string medicineListKey(long long version)
    {
        return "doctor:medicines:list:v1:" + std::to_string(version);
    }

    std::string medicineSearchKey(const std::string &keyword, long long version)
    {
        return "doctor:medicines:search:v1:" + std::to_string(version) + ":" + keyword;
    }

    int medicineCacheTtlSeconds()
    {
        return 300; // 5 分钟：版本号失效是主力（写操作即时 bump 版本），TTL 仅作兜底（漏失效 + 孤儿旧版本回收）。
                    // 取 5min 而非更长，是把"万一 invalidate 的 INCR 瞬时失败、版本没 bump"导致的脏缓存最坏滞后压到 ≤5min。
    }

    std::optional<nlohmann::json> parseMedicineArrayCache(const std::optional<std::string> &raw)
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

    namespace
    {
        std::optional<long long> readCacheVersion(RedisClient &redis)
        {
            std::optional<std::string> raw = redis.get(medicineCacheVersionKey());
            if (!raw.has_value())
            {
                return defaultMedicineCacheVersion();
            }

            try
            {
                long long version = std::stoll(raw.value());
                if (version < 0)
                {
                    return defaultMedicineCacheVersion();
                }
                return version;
            }
            catch (...)
            {
                return defaultMedicineCacheVersion();
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

    std::optional<nlohmann::json> readMedicineListCache(long long version)
    {
        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return std::nullopt;
        }
        return parseMedicineArrayCache(redis.get(medicineListKey(version)));
    }

    std::optional<nlohmann::json> readMedicineSearchCache(const std::string &keyword, long long version)
    {
        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return std::nullopt;
        }
        return parseMedicineArrayCache(redis.get(medicineSearchKey(keyword, version)));
    }

    bool writeMedicineListCache(long long version, const nlohmann::json &medicines)
    {
        if (!medicines.is_array())
        {
            return false;
        }

        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return false;
        }

        return redis.setEx(medicineListKey(version), medicineCacheTtlSeconds(), medicines.dump());
    }

    bool writeMedicineSearchCache(const std::string &keyword, long long version, const nlohmann::json &medicines)
    {
        if (!medicines.is_array())
        {
            return false;
        }

        RedisClient &redis = RedisClient::instance();
        if (!redis.enabled())
        {
            return false;
        }

        return redis.setEx(medicineSearchKey(keyword, version), medicineCacheTtlSeconds(), medicines.dump());
    }

    void invalidateMedicineCache()
    {
        RedisClient &redis = RedisClient::instance();
        if (redis.enabled())
        {
            redis.incr(medicineCacheVersionKey());
        }
    }
}
