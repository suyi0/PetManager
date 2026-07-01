#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>

struct redisContext; // 前向声明，避免在头文件暴露 hiredis

// RedisSubscription：一条独立的订阅连接 + 后台线程，收到消息回调 onMessage。
// 用于 WebSocket 跨实例广播：任一实例 PUBLISH，所有实例的订阅线程都会触发本地推送。
// 断线会自动重连；析构或 stop() 时安全退出（shutdown(fd) 打断阻塞读）。
class RedisSubscription
{
public:
    ~RedisSubscription();
    void stop();

private:
    friend class RedisClient;
    RedisSubscription() = default;

    void loop();

    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<int> fd_{-1}; // 订阅连接 socket fd，stop 时用于打断阻塞读
    std::string channel_;
    std::string host_;
    int port_ = 6379;
    std::string password_;
    // 代表 Redis 数据库索引，默认为 0。Redis 支持多个逻辑数据库，每个数据库都有一个唯一的索引号。
    int db_ = 0;
    std::function<void(const std::string &)> onMessage_;
};



// RedisClient：对 hiredis 的薄封装，作为系统的内存中转站。
//
// 设计要点：
// 1. 单例 + 进程级配置（host/port/pass/db）一次性从环境变量读取。
// 2. 每个工作线程各持一个 thread_local 连接（hiredis 的 redisContext 非线程安全），
//    避免在限流等热路径上加全局锁；连接惰性建立、出错自动重连。
// 3. 优雅降级：任何一步失败都返回失败/空值，由调用方回退到原有内存逻辑，
//    Redis 绝不能成为"挂了就登不上"的单点。
//
// 当前覆盖能力：键值带 TTL（验证码）+ 有序集合滑动窗口（限流）。
class RedisClient
{
public:
    static RedisClient &instance();

    // 在 main 启动时调用一次：读取环境变量并做一次连通性探测。
    // 探测失败不抛异常，只把 enabled_ 置为相应状态并打印日志。
    void init();

    // Redis 是否被启用且初始探测通过。调用方据此决定是否走 Redis 路径。
    bool enabled() const { return enabled_; }

    // ---- 键值（验证码） ----
    // SET：写入不带过期时间的字符串。成功返回 true。
    bool set(const std::string &key, const std::string &value);
    // SETEX：写入带过期时间的字符串。成功返回 true。
    bool setEx(const std::string &key, int ttlSeconds, const std::string &value);
    // GET：取字符串；键不存在返回 std::nullopt；连接错误也返回 nullopt。
    std::optional<std::string> get(const std::string &key);
    // DEL：删除键。成功返回 true。
    bool del(const std::string &key);
    // INCR：原子自增并返回新值；键不存在时从 0 自增为 1。Redis 出错返回 nullopt。
    std::optional<long long> incr(const std::string &key);
    // EXPIRE：给键设置过期秒数。成功返回 true。
    bool expire(const std::string &key, int ttlSeconds);

    // ---- 有序集合滑动窗口（限流） ----
    // 剪掉窗口外的成员后返回当前窗口内成员数；失败返回 nullopt。
    std::optional<long long> zWindowCount(const std::string &key,
                                          long long cutoffScoreMs);
    // 向窗口追加一次命中（score=毫秒时间戳，member 需唯一），并刷新 TTL。
    bool zWindowAdd(const std::string &key, long long scoreMs,
                    const std::string &member, int ttlSeconds);

    // 原子滑动窗口命中（限流用）：单条 Lua 内完成"剪窗口→计数→未超限则记录→刷 TTL"，
    // 消除 check 与 record 分离造成的并发穿透。admitted=是否放行，count=操作后窗口内计数。
    // member 需唯一且不含空格。Redis 出错返回 nullopt（调用方据此降级）。
    struct WindowHit
    {
        bool admitted;
        long long count;
    };
    std::optional<WindowHit> zWindowHit(const std::string &key, long long cutoffScoreMs,
                                        long long nowScoreMs, const std::string &member,
                                        int ttlSeconds, long long limit);

    // ---- 分布式锁（定时任务多实例去重） ----
    // SET key value NX EX ttl：三态返回，让调用方区分"被别人占用"与"Redis 出错"。
    //   - true   ：抢到锁（仅一个实例成功）
    //   - false  ：键已存在（锁被别人持有）
    //   - nullopt：Redis 不可用或命令出错（调用方应按"降级"处理，而非误判为被占用）
    std::optional<bool> setNxEx(const std::string &key, int ttlSeconds, const std::string &value);
    // Lua compare-and-del：仅当 key 当前值 == expectedValue 时才 DEL（token 安全释放锁，
    // 避免误删"持锁超 TTL 后别人重新拿到的同名锁"）。删除成功返回 true。
    bool compareAndDel(const std::string &key, const std::string &expectedValue);

    // ---- 发布/订阅（WebSocket 跨实例广播） ----
    // 向频道发布一条消息；返回是否成功投递到 Redis。
    bool publish(const std::string &channel, const std::string &message);
    // 订阅频道；每条消息触发 onMessage(payload)。返回订阅句柄，析构即退订。
    // Redis 未启用时返回 nullptr，调用方据此回退为单实例本地逻辑。
    std::shared_ptr<RedisSubscription> subscribe(
        const std::string &channel,
        std::function<void(const std::string &)> onMessage);

private:
    RedisClient() = default;
    ~RedisClient();
    RedisClient(const RedisClient &) = delete;
    RedisClient &operator=(const RedisClient &) = delete;

    // 取当前线程的连接，必要时建立/重连；不可用返回 nullptr。
    redisContext *acquire();
    // 关闭并清理当前线程连接（出错后强制下次重连）。
    void release(redisContext *&ctx);

    bool enabled_ = false;
    std::string host_ = "127.0.0.1";
    int port_ = 6379;
    std::string password_;
    int db_ = 0;
    int timeoutMs_ = 200; // 连接/命令超时，热路径上失败要快
};
