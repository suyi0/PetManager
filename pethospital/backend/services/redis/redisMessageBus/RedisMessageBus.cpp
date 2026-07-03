#include "RedisMessageBus.h"

#include "../RedisClient.h"

#include <hiredis/hiredis.h>
#include <sys/socket.h>

#include <chrono>

RedisMessageBus &RedisMessageBus::instance()
{
    static RedisMessageBus inst;
    return inst;
}

void RedisMessageBus::subscribe(const std::string &channel, std::function<void(const std::string &)> handler)
{
    bool isNewChannel = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto &list = handlers_[channel];
        isNewChannel = list.empty();
        list.push_back(std::move(handler));
    }

    // Redis 未启用：只登记回调、不起线程；active() 保持 false，调用方走本地回退。
    if (!RedisClient::instance().enabled())
    {
        return;
    }

    // 首次注册：惰性启动总线线程（频道已入表，loop 会带上它）。
    if (!running_.exchange(true))
    {
        thread_ = std::thread([this] { loop(); });
        return;
    }

    // 总线已在运行且出现新频道：打断阻塞读，循环顶部按当前完整频道集重连重订阅。
    if (isNewChannel)
    {
        resubscribe_.store(true);
        interrupt();
    }
}

void RedisMessageBus::stop()
{
    if (!running_.exchange(false))
    {
        return;
    }
    interrupt();
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void RedisMessageBus::interrupt()
{
    const int fd = fd_.load();
    if (fd >= 0)
    {
        ::shutdown(fd, SHUT_RDWR);
    }
}

void RedisMessageBus::loop()
{
    while (running_.load())
    {
        // 建独立订阅连接（connect + AUTH + SELECT 由 RedisClient 统一负责；
        // 订阅连接进入 subscribe 模式后不能发普通命令，故不复用工作连接）。
        redisContext *ctx = RedisClient::instance().createSubscriberConnection();
        if (!ctx)
        {
            // 连接失败：稍后重试（除非已被要求停止）。
            for (int i = 0; i < 10 && running_.load(); ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        // 快照当前完整频道集；本轮已是最新，清掉重订阅标志
        //（之后再有新频道会重新置位并打断本轮阻塞读）。
        std::vector<std::string> channels;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            channels.reserve(handlers_.size());
            for (const auto &entry : handlers_)
            {
                channels.push_back(entry.first);
            }
        }
        resubscribe_.store(false);

        if (channels.empty())
        {
            // 理论不可达（subscribe 先登记频道再启动线程），防御处理。
            redisFree(ctx);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // 一次 SUBSCRIBE 订阅全部频道。首条确认回复在此读取，
        // 其余每频道一条确认（["subscribe", ch, n]）由下方读循环按类型过滤掉。
        std::vector<const char *> argv;
        argv.reserve(channels.size() + 1);
        argv.push_back("SUBSCRIBE");
        for (const auto &ch : channels)
        {
            argv.push_back(ch.c_str());
        }
        redisReply *sub = static_cast<redisReply *>(
            redisCommandArgv(ctx, static_cast<int>(argv.size()), argv.data(), nullptr));
        if (!sub)
        {
            redisFree(ctx);
            continue;
        }
        freeReplyObject(sub);

        fd_.store(ctx->fd);

        // 阻塞读消息并分发，直到断线 / stop() / 需要重订阅。
        while (running_.load() && !resubscribe_.load())
        {
            redisReply *reply = nullptr;
            if (redisGetReply(ctx, reinterpret_cast<void **>(&reply)) != REDIS_OK || !reply)
            {
                break; // 连接断开或被 shutdown 打断
            }
            // 消息格式：["message", channel, payload]（订阅确认为 "subscribe"，被过滤）
            if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3 &&
                reply->element[0]->str &&
                std::string(reply->element[0]->str) == "message")
            {
                const std::string channel(reply->element[1]->str ? reply->element[1]->str : "",
                                          reply->element[1]->len);
                const std::string payload(reply->element[2]->str ? reply->element[2]->str : "",
                                          reply->element[2]->len);
                dispatch(channel, payload);
            }
            freeReplyObject(reply);
        }

        fd_.store(-1);
        redisFree(ctx);
        // 仍在运行 → 断线或重订阅，循环顶部重连。
    }
}

void RedisMessageBus::dispatch(const std::string &channel, const std::string &payload)
{
    std::vector<std::function<void(const std::string &)>> handlers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = handlers_.find(channel);
        if (it == handlers_.end())
        {
            return;
        }
        handlers = it->second; // 拷贝到锁外执行，避免 handler 内再碰总线时死锁
    }

    for (const auto &handler : handlers)
    {
        try
        {
            handler(payload);
        }
        catch (...)
        {
            // 回调异常不拖垮总线分发线程。
        }
    }
}
