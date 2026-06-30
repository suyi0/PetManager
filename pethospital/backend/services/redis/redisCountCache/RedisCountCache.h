#pragma once

#include <functional>
#include <string>

// 读穿透的 int COUNT 缓存：持续增长的全局 COUNT(*) 用短 TTL 缓存，命中直接返回，
// 未命中（或 Redis 未启用）才 compute() 查库并（启用时）回填。计数作为筛选页徽章，
// 几十秒的轻微滞后可接受；Redis 不可用时退化为直接 compute()，无副作用。
namespace RedisCountCache
{
    int cached(const std::string &cacheKey, int ttlSeconds, const std::function<int()> &compute);
}
