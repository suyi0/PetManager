#pragma once

#include <string>

// 分布式锁的统一封装：把散落在迁移 / 定时任务 / 预约槽位里的 `SET NX EX` 三态获取 +
// RAII 释放收口到一处。各自的业务策略（等待 / 不漏跑 / 不误拒 / 降级）仍留在调用方。
//
// 锁安全：tryAcquire 用随机 token 作为锁值，释放走 Lua compare-and-del（仅当锁仍是自己
// 持有时才删），避免"持锁超 TTL 后别人重新拿到同名锁、旧持有者析构却把它删掉"的误删。

// 自动释放的锁句柄（move-only）：析构 / 显式 release 时做 token 安全释放。
class RedisLockGuard
{
    friend class RedisLock;

public:
    RedisLockGuard() = default;
    ~RedisLockGuard();

    RedisLockGuard(RedisLockGuard &&other) noexcept;
    RedisLockGuard &operator=(RedisLockGuard &&other) noexcept;
    RedisLockGuard(const RedisLockGuard &) = delete;
    RedisLockGuard &operator=(const RedisLockGuard &) = delete;

    bool held() const { return !key_.empty(); }
    void release(); // 提前释放（token 安全；可重复调用）

private:
    RedisLockGuard(std::string key, std::string token);

    std::string key_;
    std::string token_;
};

class RedisLock
{
public:
    enum class Outcome
    {
        Acquired,    // 抢到（tryAcquire 时 guard 持锁，析构 token 安全释放）
        Contended,   // 已被别人持有
        Unavailable, // Redis 不可用 / 出错 —— 调用方应"降级"，而非当作被占用
    };

    struct Result
    {
        Outcome outcome = Outcome::Unavailable;
        RedisLockGuard guard; // 仅 Acquired 时持锁，其余为空
    };

    // 抢一把"会自动释放"的锁（token 安全）。用于：迁移启动锁、预约槽位锁。
    static Result tryAcquire(const std::string &key, int ttlSeconds);

    // 占一个"靠 TTL 回收、不主动释放"的位（claim）。用于：定时任务多实例时段去重。
    // value 为信息性内容（如 pid），不参与 token 安全释放。
    static Outcome tryClaim(const std::string &key, int ttlSeconds, const std::string &value);
};
