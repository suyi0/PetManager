#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace MedicineRedisCache
{
    std::string medicineCacheVersionKey();
    long long defaultMedicineCacheVersion();
    std::string medicineListKey(long long version);
    std::string medicineSearchKey(const std::string &keyword, long long version);
    int medicineCacheTtlSeconds();

    std::optional<nlohmann::json> parseMedicineArrayCache(const std::optional<std::string> &raw);

    // 取当前缓存版本（Redis 不可用返回 nullopt）。调用方应在"读缓存/查库之前"抓一次，
    // 读与写用同一个版本——避免写缓存时重读版本造成的 TOCTOU（把旧数据写进新版本 key）。
    std::optional<long long> currentCacheVersion();
    std::optional<nlohmann::json> readMedicineListCache(long long version);
    std::optional<nlohmann::json> readMedicineSearchCache(const std::string &keyword, long long version);
    bool writeMedicineListCache(long long version, const nlohmann::json &medicines);
    bool writeMedicineSearchCache(const std::string &keyword, long long version, const nlohmann::json &medicines);
    void invalidateMedicineCache();
}
