#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// 统一的 Redis Pub/Sub 订阅总线：全进程一条订阅连接 + 一个分发线程。
//
// 背景：原先每个广播器各持一个 RedisSubscription（独立连接 + 独立线程），
// 频道每 +1 → 连接 +1、线程 +1、样板 +1 份。总线把所有频道收敛到一次
// SUBSCRIBE ch1 ch2 ...，按 channel 查表分发；新增频道只需一行 subscribe()。
//
// handler 约定：在总线线程【串行】执行，必须轻量（置 pending 标志 / notify
// 条件变量级别，不做 IO）；重活留在各广播器自己的广播线程。
// 设计文档：项目根 REDIS-MESSAGE-BUS.md。
class RedisMessageBus
{
public:
    static RedisMessageBus &instance();

    // 注册频道回调；首次调用惰性启动总线线程（Redis 未启用则不启动，active() 保持 false）。
    // 总线运行后注册新频道会自动打断并按当前完整频道集重订阅。
    void subscribe(const std::string &channel, std::function<void(const std::string &)> handler);

    // 总线是否在运行（Redis 可用且订阅线程已启动）。
    // 广播器以此决定走 publish（跨实例）还是本地直触（替代原 subscription_ 非空判断）。
    bool active() const { return running_.load(); }

    // 优雅关停：置停标志 + shutdown(fd) 打断阻塞读 + join。可重复调用。
    void stop();

private:
    RedisMessageBus() = default;
    ~RedisMessageBus() { stop(); }
    RedisMessageBus(const RedisMessageBus &) = delete;
    RedisMessageBus &operator=(const RedisMessageBus &) = delete;

    // 订阅线程主体：连接 → SUBSCRIBE 全部频道 → 阻塞读消息分发；断线自动重连。
    void loop();
    // 按频道分发一条消息（handler 拷贝到锁外执行；异常被吞掉，不拖垮总线）。
    void dispatch(const std::string &channel, const std::string &payload);
    // shutdown(fd) 打断阻塞读（stop / 运行期重订阅共用）。
    void interrupt();

    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> resubscribe_{false}; // 运行期新增频道 → 重连并重订阅
    std::atomic<int> fd_{-1};              // 当前订阅连接 socket fd，打断阻塞读用
    mutable std::mutex mutex_;             // 保护 handlers_
    std::unordered_map<std::string, std::vector<std::function<void(const std::string &)>>> handlers_;
};
