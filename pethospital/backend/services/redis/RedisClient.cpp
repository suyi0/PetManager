#include "RedisClient.h"

#include <hiredis/hiredis.h>
#include <sys/socket.h>

#include <cstdlib>
#include <cstdarg>
#include <iostream>
#include <string>
#include <chrono>

namespace
{
    // 去掉环境变量值可能带的引号/空白，沿用项目里 DB 配置的习惯。
    std::string trimEnv(const char *raw)
    {
        if (!raw)
        {
            return std::string();
        }
        std::string v(raw);
        size_t start = v.find_first_not_of(" \t\"'");
        size_t end = v.find_last_not_of(" \t\"'");
        if (start == std::string::npos)
        {
            return std::string();
        }
        return v.substr(start, end - start + 1);
    }

    bool isFalsy(const std::string &v)
    {
        return v == "0" || v == "false" || v == "FALSE" || v == "no" || v == "off";
    }

    // RAII 托管 redisReply*：析构时自动 freeReplyObject，
    // 省掉每个命令方法里手动释放（以及"先 free 再 return/打印"的易错写法）。
    // 以 void* 构造，配合 RedisClient::command()（返回 void* 以隔离 hiredis）。
    struct ReplyPtr
    {
        redisReply *r;
        explicit ReplyPtr(void *p) : r(static_cast<redisReply *>(p)) {}
        ~ReplyPtr() { if (r) freeReplyObject(r); }
        ReplyPtr(const ReplyPtr &) = delete;
        ReplyPtr &operator=(const ReplyPtr &) = delete;
        redisReply *operator->() const { return r; }
        explicit operator bool() const { return r != nullptr; }
    };
}

// 获取全局唯一单例（首次调用时构造，线程安全的局部静态初始化）。
RedisClient &RedisClient::instance()
{
    static RedisClient inst;
    return inst;
}

// 析构：连接是 thread_local 的，随各线程退出自动释放，这里不做集中回收。
RedisClient::~RedisClient()
{
    // thread_local 连接在各自线程退出时释放；此处无需统一回收。
}

void RedisClient::init()
{
    // REDIS_ENABLED=0/false 可显式关闭（强制全程走内存回退）。
    std::string enabledEnv = trimEnv(std::getenv("REDIS_ENABLED"));
    if (!enabledEnv.empty() && isFalsy(enabledEnv))
    {
        enabled_ = false;
        std::cout << "ℹ️  Redis disabled via REDIS_ENABLED. Using in-memory fallback." << std::endl;
        return;
    }

    std::string host = trimEnv(std::getenv("REDIS_HOST"));
    if (!host.empty())
    {
        host_ = host;
    }
    std::string portEnv = trimEnv(std::getenv("REDIS_PORT"));
    if (!portEnv.empty())
    {
        try
        {
            port_ = std::stoi(portEnv);
        }
        catch (...)
        {
            std::cerr << "⚠️  Invalid REDIS_PORT '" << portEnv << "', using " << port_ << std::endl;
        }
    }
    password_ = trimEnv(std::getenv("REDIS_PASS"));
    std::string dbEnv = trimEnv(std::getenv("REDIS_DB"));
    if (!dbEnv.empty())
    {
        try
        {
            db_ = std::stoi(dbEnv);
        }
        catch (...)
        {
            std::cerr << "⚠️  Invalid REDIS_DB '" << dbEnv << "', using " << db_ << std::endl;
        }
    }

    // 启动期做一次连通性探测：成功才启用 Redis 路径。
    enabled_ = true;
    redisContext *ctx = acquire();
    if (!ctx)
    {
        enabled_ = false;
        std::cerr << "⚠️  Redis unavailable at " << host_ << ":" << port_
                  << ". Falling back to in-memory store." << std::endl;
        return;
    }

    ReplyPtr reply(redisCommand(ctx, "PING"));
    if (!reply)
    {
        enabled_ = false;
        std::cerr << "⚠️  Redis PING failed (no reply). Falling back to in-memory store." << std::endl;
        return;
    }
    // 必须校验是真的 PONG：NOAUTH/WRONGPASS 等也是“有返回”的错误回复，
    // 不能当成连接成功，否则后续命令全被拒、却以为 Redis 可用（锁会失败关闭）。
    if (reply->type == REDIS_REPLY_ERROR)
    {
        std::string err = reply->str ? reply->str : "unknown";
        enabled_ = false;
        std::cerr << "⚠️  Redis PING rejected (" << err
                  << "). Check REDIS_PASS. Falling back to in-memory store." << std::endl;
        return;
    }
    std::cout << "✅ Redis connected at " << host_ << ":" << port_ << " (db " << db_ << ")" << std::endl;
}

redisContext *RedisClient::acquire()
{
    if (!enabled_)
    {
        return nullptr;
    }

    // 每线程一个连接：避免 redisContext 的线程安全问题，也省掉热路径全局锁。
    static thread_local redisContext *tlCtx = nullptr;

    // 已有可用连接直接复用。
    if (tlCtx && !tlCtx->err)
    {
        return tlCtx;
    }
    if (tlCtx && tlCtx->err)
    {
        redisFree(tlCtx);
        tlCtx = nullptr;
    }

    struct timeval tv;
    tv.tv_sec = timeoutMs_ / 1000;
    tv.tv_usec = (timeoutMs_ % 1000) * 1000;

    // 创建连接
    redisContext *ctx = redisConnectWithTimeout(host_.c_str(), port_, tv);
    if (!ctx || ctx->err)
    {
        if (ctx)
        {
            redisFree(ctx);
        }
        return nullptr;
    }
    // 设置超时
    redisSetTimeout(ctx, tv);

    // 鉴权 + 选库；任一失败则放弃该连接。
    // 这里直接用 redisCommand（不能走 command()，否则会再次 acquire 造成递归）。
    if (!password_.empty())
    {
        ReplyPtr auth(redisCommand(ctx, "AUTH %s", password_.c_str()));
        if (!auth || auth->type == REDIS_REPLY_ERROR)
        {
            redisFree(ctx);
            return nullptr;
        }
    }
    if (db_ != 0)
    {
        ReplyPtr sel(redisCommand(ctx, "SELECT %d", db_));
        if (!sel || sel->type == REDIS_REPLY_ERROR)
        {
            redisFree(ctx);
            return nullptr;
        }
    }

    tlCtx = ctx;
    return tlCtx;
}

// 释放一个连接并把指针置空（出错连接清理用）。
void RedisClient::release(redisContext *&ctx)
{
    if (ctx)
    {
        redisFree(ctx);
        ctx = nullptr;
    }
}

// 统一执行入口：取线程连接 → 判空 → 发命令。把各操作里重复的样板收成一处。
// 连接不可用返回 nullptr；否则返回 redisReply*（调用方用 ReplyPtr 托管释放）。
void *RedisClient::command(const char *format, ...)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return nullptr;
    }
    va_list ap;
    va_start(ap, format);
    void *reply = redisvCommand(ctx, format, ap);
    va_end(ap);
    return reply;
}

bool RedisClient::setEx(const std::string &key, int ttlSeconds, const std::string &value)
{
    ReplyPtr reply(command("SETEX %s %d %b", key.c_str(), ttlSeconds, value.data(), value.size()));
    return reply && reply->type == REDIS_REPLY_STATUS; // STATUS 表示写入成功
}

bool RedisClient::ping()
{
    // NOAUTH/WRONGPASS 等错误也是非空回复，必须校验回复类型（见 init() 的教训）。
    ReplyPtr reply(command("PING"));
    return reply && reply->type == REDIS_REPLY_STATUS;
}

bool RedisClient::set(const std::string &key, const std::string &value)
{
    ReplyPtr reply(command("SET %s %b", key.c_str(), value.data(), value.size()));
    return reply && reply->type == REDIS_REPLY_STATUS;
}

std::optional<std::string> RedisClient::get(const std::string &key)
{
    ReplyPtr reply(command("GET %s", key.c_str()));
    if (reply && reply->type == REDIS_REPLY_STRING)
    {
        return std::string(reply->str, reply->len);
    }
    // REDIS_REPLY_NIL（键不存在）/ 连接错误 → nullopt
    return std::nullopt;
}

bool RedisClient::del(const std::string &key)
{
    ReplyPtr reply(command("DEL %s", key.c_str()));
    return reply && reply->type == REDIS_REPLY_INTEGER;
}

std::optional<long long> RedisClient::incr(const std::string &key)
{
    ReplyPtr reply(command("INCR %s", key.c_str()));
    if (reply && reply->type == REDIS_REPLY_INTEGER)
    {
        return reply->integer;
    }
    return std::nullopt;
}

bool RedisClient::expire(const std::string &key, int ttlSeconds)
{
    ReplyPtr reply(command("EXPIRE %s %d", key.c_str(), ttlSeconds));
    return reply && reply->type == REDIS_REPLY_INTEGER;
}

std::optional<long long> RedisClient::zWindowCount(const std::string &key,
                                                   long long cutoffScoreMs)
{
    // 先剪掉窗口外（score < cutoff）的成员，再取当前基数。
    ReplyPtr prune(command("ZREMRANGEBYSCORE %s -inf (%lld", key.c_str(), cutoffScoreMs));
    if (!prune)
    {
        return std::nullopt;
    }
    ReplyPtr card(command("ZCARD %s", key.c_str()));
    if (card && card->type == REDIS_REPLY_INTEGER)
    {
        return card->integer;
    }
    return std::nullopt;
}

bool RedisClient::zWindowAdd(const std::string &key, long long scoreMs,
                             const std::string &member, int ttlSeconds)
{
    ReplyPtr add(command("ZADD %s %lld %s", key.c_str(), scoreMs, member.c_str()));
    if (!add)
    {
        return false;
    }
    // 刷新整体 TTL，保证空窗口的键最终自动回收（仅要求命令有返回，与原逻辑一致）。
    ReplyPtr exp(command("EXPIRE %s %d", key.c_str(), ttlSeconds));
    return static_cast<bool>(exp);
}

// 窗口内成员数超过限制则拒绝加入
std::optional<RedisClient::WindowHit> RedisClient::zWindowHit(const std::string &key, long long cutoffScoreMs,
                                                              long long nowScoreMs, const std::string &member,
                                                              int ttlSeconds, long long limit)
{
    // 单条 Lua 原子完成：剪窗口 → 计数 → 未超限则记录 → 刷 TTL；返回 {admitted, count}。
    static const char *kScript =
        "redis.call('ZREMRANGEBYSCORE', KEYS[1], '-inf', '(' .. ARGV[1]) "
        "local count = redis.call('ZCARD', KEYS[1]) "
        "local limit = tonumber(ARGV[5]) "
        "local admitted = 0 "
        "if count < limit then "
        "  redis.call('ZADD', KEYS[1], ARGV[2], ARGV[3]) "
        "  count = count + 1 "
        "  admitted = 1 "
        "end "
        "redis.call('EXPIRE', KEYS[1], ARGV[4]) "
        "return {admitted, count}";
    ReplyPtr reply(command("EVAL %s 1 %s %lld %lld %s %d %lld",
                           kScript, key.c_str(), cutoffScoreMs, nowScoreMs,
                           member.c_str(), ttlSeconds, limit));
    if (reply && reply->type == REDIS_REPLY_ARRAY && reply->elements == 2 &&
        reply->element[0]->type == REDIS_REPLY_INTEGER &&
        reply->element[1]->type == REDIS_REPLY_INTEGER)
    {
        return WindowHit{reply->element[0]->integer != 0, reply->element[1]->integer};
    }
    return std::nullopt;
}

std::optional<bool> RedisClient::setNxEx(const std::string &key, int ttlSeconds, const std::string &value)
{
    ReplyPtr reply(command("SET %s %s NX EX %d", key.c_str(), value.c_str(), ttlSeconds));
    if (!reply)
    {
        return std::nullopt; // Redis 不可用/命令出错：交调用方降级，而非误判为"被占用"
    }
    if (reply->type == REDIS_REPLY_STATUS)
    {
        return true; // 抢到锁
    }
    if (reply->type == REDIS_REPLY_NIL)
    {
        return false; // 键已存在：锁被别人持有
    }
    return std::nullopt; // 其它（REDIS_REPLY_ERROR 等）→ 出错降级
}

bool RedisClient::compareAndDel(const std::string &key, const std::string &expectedValue)
{
    // 仅当当前值等于 expectedValue 时才删除，保证只释放自己持有的锁。
    static const char *kScript =
        "if redis.call('get', KEYS[1]) == ARGV[1] then return redis.call('del', KEYS[1]) else return 0 end";
    ReplyPtr reply(command("EVAL %s 1 %s %s", kScript, key.c_str(), expectedValue.c_str()));
    return reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
}

bool RedisClient::publish(const std::string &channel, const std::string &message)
{
    ReplyPtr reply(command("PUBLISH %s %b", channel.c_str(), message.data(), message.size()));
    return reply && reply->type == REDIS_REPLY_INTEGER;
}

// 建一条独立的订阅用阻塞连接（已 AUTH/SELECT，连接超时 1s、读不设超时——订阅要一直阻塞等消息）。
// 订阅连接进入 subscribe 模式后无法再发普通命令，故与工作连接（thread_local）分开。
// 失败返回 nullptr；调用方负责 redisFree。目前唯一消费者是 RedisMessageBus（统一订阅总线）。
redisContext *RedisClient::createSubscriberConnection()
{
    if (!enabled_)
    {
        return nullptr;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    redisContext *ctx = redisConnectWithTimeout(host_.c_str(), port_, tv);
    if (!ctx || ctx->err)
    {
        if (ctx)
        {
            redisFree(ctx);
        }
        return nullptr;
    }

    if (!password_.empty())
    {
        ReplyPtr auth(redisCommand(ctx, "AUTH %s", password_.c_str()));
        if (!auth || auth->type == REDIS_REPLY_ERROR)
        {
            redisFree(ctx);
            return nullptr;
        }
    }
    if (db_ != 0)
    {
        ReplyPtr sel(redisCommand(ctx, "SELECT %d", db_)); // 失败不阻断（沿用原订阅逻辑）
    }
    return ctx;
}
