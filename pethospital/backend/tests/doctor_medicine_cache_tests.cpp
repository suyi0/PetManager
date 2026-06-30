#include "../services/redis/medicineRedisCache/MedicineRedisCache.h"

#include <cassert>
#include <optional>

int main()
{
    assert(MedicineRedisCache::medicineCacheVersionKey() == "doctor:medicines:cache-version:v1");
    assert(MedicineRedisCache::defaultMedicineCacheVersion() == 0);
    assert(MedicineRedisCache::medicineListKey(0) == "doctor:medicines:list:v1:0");
    assert(MedicineRedisCache::medicineSearchKey("阿莫西林", 0) == "doctor:medicines:search:v1:0:阿莫西林");
    assert(MedicineRedisCache::medicineCacheTtlSeconds() == 300);

    assert(!MedicineRedisCache::parseMedicineArrayCache(std::nullopt).has_value());
    assert(!MedicineRedisCache::parseMedicineArrayCache(std::optional<std::string>{"not-json"}).has_value());
    assert(!MedicineRedisCache::parseMedicineArrayCache(std::optional<std::string>{"{\"id\":1}"}).has_value());

    auto parsed = MedicineRedisCache::parseMedicineArrayCache(
        std::optional<std::string>{"[{\"id\":1,\"name\":\"阿莫西林\"}]"});
    assert(parsed.has_value());
    assert(parsed->is_array());
    assert(parsed->size() == 1);
    assert((*parsed)[0]["id"] == 1);

    assert(!MedicineRedisCache::currentCacheVersion().has_value());
    assert(!MedicineRedisCache::readMedicineListCache(0).has_value());
    assert(!MedicineRedisCache::readMedicineSearchCache("阿莫西林", 0).has_value());
    assert(!MedicineRedisCache::writeMedicineListCache(0, nlohmann::json::array()));
    assert(!MedicineRedisCache::writeMedicineSearchCache("阿莫西林", 0, nlohmann::json::array()));

    return 0;
}
