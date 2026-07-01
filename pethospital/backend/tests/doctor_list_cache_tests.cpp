#include "../services/redis/doctorListCache/DoctorListCache.h"

#include <cassert>

int main()
{
    assert(DoctorListCache::doctorCacheVersionKey() == "doctor:list:cache-version:v1");
    assert(DoctorListCache::defaultDoctorCacheVersion() == 0);
    assert(DoctorListCache::doctorListKey("2026-07-01", 0) == "doctor:list:v1:2026-07-01:0");
    assert(DoctorListCache::doctorListTtlSeconds() == 30);
    assert(!DoctorListCache::currentCacheVersion().has_value());
    assert(!DoctorListCache::readDoctorListCache("2026-07-01", 0).has_value());
    assert(!DoctorListCache::writeDoctorListCache("2026-07-01", 0, nlohmann::json::array()));
    DoctorListCache::invalidateDoctorList();

    return 0;
}
