#pragma once

#include <functional>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace DoctorListCache
{
    std::string doctorCacheVersionKey();
    long long defaultDoctorCacheVersion();
    std::string doctorListKey(const std::string &date, long long version);
    int doctorListTtlSeconds();

    nlohmann::json cachedDoctorList(const std::string &date,
                                    const std::function<nlohmann::json()> &compute);
    std::optional<long long> currentCacheVersion();
    std::optional<nlohmann::json> readDoctorListCache(const std::string &date, long long version);
    bool writeDoctorListCache(const std::string &date, long long version, const nlohmann::json &doctors);
    void invalidateDoctorList();
}
