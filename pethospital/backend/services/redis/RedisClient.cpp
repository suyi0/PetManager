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

    redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "PING"));
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
        freeReplyObject(reply);
        enabled_ = false;
        std::cerr << "⚠️  Redis PING rejected (" << err
                  << "). Check REDIS_PASS. Falling back to in-memory store." << std::endl;
        return;
    }
    freeReplyObject(reply);
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
    if (!password_.empty())
    {
        redisReply *auth = static_cast<redisReply *>(redisCommand(ctx, "AUTH %s", password_.c_str()));
        if (!auth || auth->type == REDIS_REPLY_ERROR)
        {
            if (auth)
            {
                freeReplyObject(auth);
            }
            redisFree(ctx);
            return nullptr;
        }
        freeReplyObject(auth);
    }
    if (db_ != 0)
    {
        redisReply *sel = static_cast<redisReply *>(redisCommand(ctx, "SELECT %d", db_));
        if (!sel || sel->type == REDIS_REPLY_ERROR)
        {
            if (sel)
            {
                freeReplyObject(sel);
            }
            redisFree(ctx);
            return nullptr;
        }
        freeReplyObject(sel);
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

bool RedisClient::setEx(const std::string &key, int ttlSeconds, const std::string &value)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return false;
    }
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(ctx, "SETEX %s %d %b", key.c_str(), ttlSeconds, value.data(), value.size()));
    if (!reply)
    {
        return false; // 连接出错：下次 acquire 会因 ctx->err 重连
    }
    bool ok = (reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    return ok;
}

bool RedisClient::set(const std::string &key, const std::string &value)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return false;
    }
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(ctx, "SET %s %b", key.c_str(), value.data(), value.size()));
    if (!reply)
    {
        return false;
    }
    bool ok = (reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    return ok;
}

std::optional<std::string> RedisClient::get(const std::string &key)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return std::nullopt;
    }
    redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "GET %s", key.c_str()));
    if (!reply)
    {
        return std::nullopt;
    }
    std::optional<std::string> result;
    if (reply->type == REDIS_REPLY_STRING)
    {
        result = std::string(reply->str, reply->len);
    }
    // REDIS_REPLY_NIL（键不存在）→ 保持 nullopt
    freeReplyObject(reply);
    return result;
}

bool RedisClient::del(const std::string &key)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return false;
    }
    redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "DEL %s", key.c_str()));
    if (!reply)
    {
        return false;
    }
    bool ok = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return ok;
}

std::optional<long long> RedisClient::incr(const std::string &key)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return std::nullopt;
    }
    redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "INCR %s", key.c_str()));
    if (!reply)
    {
        return std::nullopt;
    }
    std::optional<long long> result;
    if (reply->type == REDIS_REPLY_INTEGER)
    {
        result = reply->integer;
    }
    freeReplyObject(reply);
    return result;
}

bool RedisClient::expire(const std::string &key, int ttlSeconds)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return false;
    }
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(ctx, "EXPIRE %s %d", key.c_str(), ttlSeconds));
    if (!reply)
    {
        return false;
    }
    bool ok = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return ok;
}

std::optional<long long> RedisClient::zWindowCount(const std::string &key,
                                                   long long cutoffScoreMs)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return std::nullopt;
    }

    // 先剪掉窗口外（score < cutoff）的成员。
    redisReply *prune = static_cast<redisReply *>(
        redisCommand(ctx, "ZREMRANGEBYSCORE %s -inf (%lld", key.c_str(), cutoffScoreMs));
    if (!prune)
    {
        return std::nullopt;
    }
    freeReplyObject(prune);

    redisReply *card = static_cast<redisReply *>(redisCommand(ctx, "ZCARD %s", key.c_str()));
    if (!card)
    {
        return std::nullopt;
    }
    std::optional<long long> count;
    if (card->type == REDIS_REPLY_INTEGER)
    {
        count = card->integer;
    }
    freeReplyObject(card);
    return count;
}

bool RedisClient::zWindowAdd(const std::string &key, long long scoreMs,
                             const std::string &member, int ttlSeconds)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return false;
    }
    redisReply *add = static_cast<redisReply *>(
        redisCommand(ctx, "ZADD %s %lld %s", key.c_str(), scoreMs, member.c_str()));
    if (!add)
    {
        return false;
    }
    freeReplyObject(add);

    // 刷新整体 TTL，保证空窗口的键最终自动回收。
    redisReply *exp = static_cast<redisReply *>(
        redisCommand(ctx, "EXPIRE %s %d", key.c_str(), ttlSeconds));
    if (!exp)
    {
        return false;
    }
    freeReplyObject(exp);
    return true;
}

std::optional<RedisClient::WindowHit> RedisClient::zWindowHit(const std::string &key, long long cutoffScoreMs,
                                                              long long nowScoreMs, const std::string &member,
                                                              int ttlSeconds, long long limit)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return std::nullopt;
    }
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
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(ctx, "EVAL %s 1 %s %lld %lld %s %d %lld",
                     kScript, key.c_str(), cutoffScoreMs, nowScoreMs,
                     member.c_str(), ttlSeconds, limit));
    if (!reply)
    {
        return std::nullopt;
    }
    std::optional<WindowHit> result;
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 2 &&
        reply->element[0]->type == REDIS_REPLY_INTEGER &&
        reply->element[1]->type == REDIS_REPLY_INTEGER)
    {
        result = WindowHit{reply->element[0]->integer != 0, reply->element[1]->integer};
    }
    freeReplyObject(reply);
    return result;
}

std::optional<bool> RedisClient::setNxEx(const std::string &key, int ttlSeconds, const std::string &value)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return std::nullopt; // Redis 不可用：交调用方降级，而非误判为"被占用"
    }
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(ctx, "SET %s %s NX EX %d", key.c_str(), value.c_str(), ttlSeconds));
    if (!reply)
    {
        return std::nullopt; // 连接/命令出错：同样降级
    }
    std::optional<bool> result;
    if (reply->type == REDIS_REPLY_STATUS)
    {
        result = true; // 抢到锁
    }
    else if (reply->type == REDIS_REPLY_NIL)
    {
        result = false; // 键已存在：锁被别人持有
    }
    // 其它（REDIS_REPLY_ERROR 等）→ 保持 nullopt（出错降级）
    freeReplyObject(reply);
    return result;
}

bool RedisClient::compareAndDel(const std::string &key, const std::string &expectedValue)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return false;
    }
    // 仅当当前值等于 expectedValue 时才删除，保证只释放自己持有的锁。
    static const char *kScript =
        "if redis.call('get', KEYS[1]) == ARGV[1] then return redis.call('del', KEYS[1]) else return 0 end";
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(ctx, "EVAL %s 1 %s %s", kScript, key.c_str(), expectedValue.c_str()));
    if (!reply)
    {
        return false;
    }
    bool deleted = (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1);
    freeReplyObject(reply);
    return deleted;
}

bool RedisClient::publish(const std::string &channel, const std::string &message)
{
    redisContext *ctx = acquire();
    if (!ctx)
    {
        return false;
    }
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(ctx, "PUBLISH %s %b", channel.c_str(), message.data(), message.size()));
    if (!reply)
    {
        return false;
    }
    bool ok = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return ok;
}

std::shared_ptr<RedisSubscription> RedisClient::subscribe(
    const std::string &channel,
    std::function<void(const std::string &)> onMessage)
{
    if (!enabled_)
    {
        return nullptr;
    }
    // 不能用 make_shared：构造函数私有，这里是 friend。
    std::shared_ptr<RedisSubscription> sub(new RedisSubscription());
    sub->channel_ = channel;
    sub->host_ = host_;
    sub->port_ = port_;
    sub->password_ = password_;
    sub->db_ = db_;
    sub->onMessage_ = std::move(onMessage);
    sub->running_ = true;
    sub->thread_ = std::thread([sub]()
                               { sub->loop(); });
    return sub;
}

// ===================== RedisSubscription =====================

// 析构即退订：确保后台线程安全退出。
RedisSubscription::~RedisSubscription()
{
    stop();
}

// 停止订阅：置停标志 + shutdown(fd) 打断阻塞读，再 join 后台线程。可重复调用。
void RedisSubscription::stop()
{
    if (!running_.exchange(false))
    {
        return;
    }
    // 打断订阅线程里阻塞的 redisGetReply。
    int fd = fd_.load();
    if (fd >= 0)
    {
        ::shutdown(fd, SHUT_RDWR);
    }
    if (thread_.joinable())
    {
        thread_.join();
    }
}

// 订阅线程主体：连接→鉴权→SUBSCRIBE→阻塞读消息并回调；断线自动重连，直到 stop()。
void RedisSubscription::loop()
{
    while (running_.load())
    {
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
            // 连接失败：稍后重试（除非已被要求停止）。
            for (int i = 0; i < 10 && running_.load(); ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        if (!password_.empty())
        {
            redisReply *auth = static_cast<redisReply *>(redisCommand(ctx, "AUTH %s", password_.c_str()));
            if (!auth || auth->type == REDIS_REPLY_ERROR)
            {
                if (auth)
                {
                    freeReplyObject(auth);
                }
                redisFree(ctx);
                continue;
            }
            freeReplyObject(auth);
        }
        if (db_ != 0)
        {
            redisReply *sel = static_cast<redisReply *>(redisCommand(ctx, "SELECT %d", db_));
            if (sel)
            {
                freeReplyObject(sel);
            }
        }

        redisReply *sub = static_cast<redisReply *>(redisCommand(ctx, "SUBSCRIBE %s", channel_.c_str()));
        if (!sub)
        {
            redisFree(ctx);
            continue;
        }
        freeReplyObject(sub);

        fd_.store(ctx->fd);

        // 阻塞读消息，直到出错或被 stop() 打断。
        while (running_.load())
        {
            redisReply *reply = nullptr;
            if (redisGetReply(ctx, reinterpret_cast<void **>(&reply)) != REDIS_OK || !reply)
            {
                break; // 连接断开或被 shutdown 打断
            }
            // 消息格式：["message", channel, payload]
            if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3 &&
                reply->element[0]->str &&
                std::string(reply->element[0]->str) == "message")
            {
                std::string payload(reply->element[2]->str ? reply->element[2]->str : "",
                                    reply->element[2]->len);
                if (onMessage_)
                {
                    try
                    {
                        onMessage_(payload);
                    }
                    catch (...)
                    {
                        // 回调异常不应拖垮订阅线程。
                    }
                }
            }
            freeReplyObject(reply);
        }

        fd_.store(-1);
        redisFree(ctx);
        // 若仍在运行说明是断线，循环顶部会重连。
    }
}
