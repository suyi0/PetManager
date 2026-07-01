#pragma once

#include <functional>
#include <string>

#include <nlohmann/json.hpp>

// 通用短 TTL JSON 读穿透缓存：命中返回缓存的 JSON；未命中（或 Redis 未启用）则 compute() 查库、
// （启用时）SETEX 回填。具体 key、TTL、主动失效策略由各领域缓存模块封装。
namespace RedisJsonCache
{
    nlohmann::json cached(const std::string &cacheKey, int ttlSeconds,
                          const std::function<nlohmann::json()> &compute);
}
