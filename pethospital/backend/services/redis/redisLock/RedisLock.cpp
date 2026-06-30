#include "RedisLock.h"

#include <atomic>
#include <chrono>
#include <utility>

#include <unistd.h>

#include "../RedisClient.h"

namespace
{
    // 进程内唯一 + 跨进程足够唯一的锁 token：pid + 高精度时间 + 自增序号（无空格）。
    std::string generateLockToken()
    {
        static std::atomic<unsigned long long> counter{0};
        const long long now =
            std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return std::to_string(static_cast<long long>(::getpid())) + "-" +
               std::to_string(now) + "-" +
               std::to_string(counter.fetch_add(1));
    }
}

RedisLockGuard::RedisLockGuard(std::string key, std::string token)
    : key_(std::move(key)), token_(std::move(token))
{
}

RedisLockGuard::RedisLockGuard(RedisLockGuard &&other) noexcept
    : key_(std::move(other.key_)), token_(std::move(other.token_))
{
    other.key_.clear();
    other.token_.clear();
}

RedisLockGuard &RedisLockGuard::operator=(RedisLockGuard &&other) noexcept
{
    if (this != &other)
    {
        release();
        key_ = std::move(other.key_);
        token_ = std::move(other.token_);
        other.key_.clear();
        other.token_.clear();
    }
    return *this;
}

RedisLockGuard::~RedisLockGuard()
{
    release();
}

void RedisLockGuard::release()
{
    if (key_.empty())
    {
        return;
    }
    // token 安全释放：仅当锁仍是自己持有时才删（防误删超时后别人重新拿到的同名锁）。
    RedisClient::instance().compareAndDel(key_, token_);
    key_.clear();
    token_.clear();
}

RedisLock::Result RedisLock::tryAcquire(const std::string &key, int ttlSeconds)
{
    const std::string token = generateLockToken();
    std::optional<bool> acquired = RedisClient::instance().setNxEx(key, ttlSeconds, token);
    if (!acquired.has_value())
    {
        return Result{Outcome::Unavailable, RedisLockGuard{}};
    }
    if (acquired.value())
    {
        return Result{Outcome::Acquired, RedisLockGuard{key, token}};
    }
    return Result{Outcome::Contended, RedisLockGuard{}};
}

RedisLock::Outcome RedisLock::tryClaim(const std::string &key, int ttlSeconds, const std::string &value)
{
    std::optional<bool> claimed = RedisClient::instance().setNxEx(key, ttlSeconds, value);
    if (!claimed.has_value())
    {
        return Outcome::Unavailable;
    }
    return claimed.value() ? Outcome::Acquired : Outcome::Contended;
}
