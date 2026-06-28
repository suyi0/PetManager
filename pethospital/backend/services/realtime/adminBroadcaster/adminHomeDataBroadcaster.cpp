#include "adminHomeDataBroadcaster.h"

#include <chrono>
#include <iostream>
#include <utility>

#include "../../../controllers/modules/admin/adminHandler.h"
#include "../../../utils/Utils.h"
#include "../../redis/RedisClient.h"

namespace
{
    // 超管首页变更的跨实例广播频道。
    constexpr const char *kAdminHomeChannel = "realtime:admin-home";
}

// 获取超级管理员首页实时广播器单例，保证整个服务只维护一个广播线程和连接池。
AdminHomeDataBroadcaster &AdminHomeDataBroadcaster::instance()
{
    static AdminHomeDataBroadcaster instance;
    return instance;
}

// 启动首页数据广播线程，并保存数据库管理器供后续查询首页统计数据使用。
void AdminHomeDataBroadcaster::start(std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    dbManager_ = std::move(dbManager);

    // exchange(true) 可以避免重复启动多个广播线程。
    if (running_.exchange(true))
    {
        return;
    }

    broadcast_thread_ = std::thread([this]() { run(); });

    // 订阅跨实例广播：任一实例发布变更，本实例也会触发一次本地推送。
    // Redis 未启用时 subscribe 返回 nullptr，自动退化为单实例本地通知。
    subscription_ = RedisClient::instance().subscribe(
        kAdminHomeChannel, [this](const std::string &) { triggerLocalPush(); });
}

// 停止广播线程；唤醒等待中的线程后等待其安全退出。
void AdminHomeDataBroadcaster::stop()
{
    if (subscription_)
    {
        subscription_->stop();
        subscription_.reset();
    }

    running_ = false;
    broadcast_cv_.notify_all();

    if (broadcast_thread_.joinable())
    {
        broadcast_thread_.join();
    }
}

// 新增一个 WebSocket 连接，并唤醒广播线程尽快推送一次最新数据。
void AdminHomeDataBroadcaster::addConnection(crow::websocket::connection *conn)
{
    if (!conn)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.insert(conn);
        pending_update_ = true;
    }

    broadcast_cv_.notify_all(); // 唤醒广播线程
}

// 移除已关闭或异常的 WebSocket 连接，避免后续继续向失效连接推送。
void AdminHomeDataBroadcaster::removeConnection(crow::websocket::connection *conn)
{
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(conn);
}

// 服务关闭或路由卸载时主动关闭全部连接，并清空连接池。
void AdminHomeDataBroadcaster::closeAllConnections(const std::string &reason)
{
    std::vector<crow::websocket::connection *> connections;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections.assign(connections_.begin(), connections_.end());
        connections_.clear();
    }

    for (auto *conn : connections)
    {
        if (!conn)
        {
            continue;
        }

        try
        {
            conn->close(reason);
        }
        catch (...)
        {
            // 忽略关闭连接时的异常，避免影响服务关闭流程。
        }
    }
}

// 业务数据发生变化时调用。多实例下先发布到 Redis 频道，让所有实例（含本实例）
// 都收到并各自推送给本地连接；单实例或 Redis 不可用时直接触发本地推送。
void AdminHomeDataBroadcaster::notifyHomeDataChanged()
{
    if (subscription_ && RedisClient::instance().publish(kAdminHomeChannel, "1"))
    {
        return; // 本实例会通过自身订阅收到消息并触发本地推送
    }
    triggerLocalPush();
}

// 仅唤醒本实例广播线程做一次推送，不再发布，避免订阅回环。
void AdminHomeDataBroadcaster::triggerLocalPush()
{
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        pending_update_ = true;
    }
    broadcast_cv_.notify_all();
}

// 后台监管循环：没有变化时休眠；多个并发通知会在短暂合并窗口内压缩成一次推送。
void AdminHomeDataBroadcaster::run()
{
    while (running_)
    {
        std::unique_lock<std::mutex> waitLock(connections_mutex_);
        // 线程在这里进入休眠等待状态
        // 这个调用会让线程释放锁并进入休眠，直到被其他线程通过 notify_all() 唤醒，
        // 并且等待的条件（!running_ || pending_update_）为真时才会继续执行。
        broadcast_cv_.wait(waitLock, [this]() { return !running_ || pending_update_; });
        // 当被唤醒后，继续执行推送逻辑
        if (!running_)
        {
            break;  // 退出循环
        }

        pending_update_ = false;

        // 没有订阅者时不查询数据库，减少无意义的后台开销；下一次打开首页会触发首帧推送。
        if (connections_.empty())
        {
            continue;
        }
        waitLock.unlock();  // 释放锁

        // 合并短时间内连续发生的多个业务更新，避免一个接口批量操作触发多次查库推送。
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            pending_update_ = false;
        }

        pushHomeData();
    }
}

// 查询超级管理员首页数据并发送给当前全部 WebSocket 连接。
void AdminHomeDataBroadcaster::pushHomeData()
{
    std::vector<crow::websocket::connection *> connections;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        // connections.assign() 会创建一个新数组，并把 connections_ 中的元素复制到新数组中。
        connections.assign(connections_.begin(), connections_.end());
    }

    if (connections.empty() || !dbManager_)
    {
        return;
    }

    try
    {
        adminHandler handler(dbManager_);
        nlohmann::json message = {
            {"event", "homeData"},
            {"version", 1},
            {"sentAt", getCreateTime()},
            {"data", handler.buildHomeData()}};
        const std::string payload = message.dump();

        // 先记录发送失败的连接，循环结束后再统一从连接池移除。
        std::unordered_set<crow::websocket::connection *> failedConnections;
        for (auto *conn : connections)
        {
            if (!conn)
            {
                continue;   // 忽略无效连接
            }

            // 有效连接，尝试发送数据
            try
            {
                conn->send_text(payload);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Admin homeData WebSocket send failed: " << e.what() << std::endl;
                failedConnections.insert(conn); // 记录发送失败的连接
            }
        }

        if (!failedConnections.empty())
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            for (auto *conn : failedConnections)
            {
                connections_.erase(conn);   // 从连接池中移除发送失败的连接
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Admin homeData broadcast failed: " << e.what() << std::endl;
    }
}
