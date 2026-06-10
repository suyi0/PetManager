#include "financeHomeDataBroadcaster.h"

#include <chrono>
#include <iostream>
#include <utility>

#include "../../../controllers/modules/finance/financeHandler.h"
#include "../../../utils/Utils.h"

// 获取财务端首页实时广播器单例，保证整个服务只维护一个广播线程和连接池。
FinanceHomeDataBroadcaster &FinanceHomeDataBroadcaster::instance()
{
    static FinanceHomeDataBroadcaster instance;
    return instance;
}

// 启动财务首页数据广播线程，并保存数据库管理器供后续查询财务统计数据使用。
void FinanceHomeDataBroadcaster::start(std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    dbManager_ = std::move(dbManager);

    // exchange(true) 可以避免重复启动多个广播线程。
    if (running_.exchange(true))
    {
        return;
    }

    broadcast_thread_ = std::thread([this]()
                                    { run(); });
}

// 停止广播线程；唤醒等待中的线程后等待其安全退出。
void FinanceHomeDataBroadcaster::stop()
{
    running_ = false;
    broadcast_cv_.notify_all();

    if (broadcast_thread_.joinable())
    {
        broadcast_thread_.join();
    }
}

// 新增一个 WebSocket 连接，并唤醒广播线程尽快推送一次最新数据。
void FinanceHomeDataBroadcaster::addConnection(crow::websocket::connection *conn)
{
    if (!conn)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.insert(conn);
    }

    broadcast_cv_.notify_all();
}

// 移除已关闭或异常的 WebSocket 连接，避免后续继续向失效连接推送。
void FinanceHomeDataBroadcaster::removeConnection(crow::websocket::connection *conn)
{
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(conn);
}

// 服务关闭或路由卸载时主动关闭全部连接，并清空连接池。
void FinanceHomeDataBroadcaster::closeAllConnections(const std::string &reason)
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

// 后台广播循环：有连接时每 5 秒推送一次财务首页数据，也会在新连接加入时被唤醒。
void FinanceHomeDataBroadcaster::run()
{
    while (running_)
    {
        std::unique_lock<std::mutex> waitLock(connections_mutex_);
        broadcast_cv_.wait_for(waitLock, std::chrono::seconds(5));
        if (!running_)
        {
            break;
        }

        // 没有订阅者时不查询数据库，减少无意义的后台开销。
        if (connections_.empty())
        {
            continue;
        }
        waitLock.unlock();

        pushHomeData();
    }
}

// 查询财务端首页数据并发送给当前全部 WebSocket 连接。
void FinanceHomeDataBroadcaster::pushHomeData()
{
    std::vector<crow::websocket::connection *> connections;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections.assign(connections_.begin(), connections_.end());
    }

    if (connections.empty() || !dbManager_)
    {
        return;
    }

    try
    {
        financeHandler handler(dbManager_);
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
                continue;
            }

            try
            {
                conn->send_text(payload);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Finance homeData WebSocket send failed: " << e.what() << std::endl;
                failedConnections.insert(conn);
            }
        }

        if (!failedConnections.empty())
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            for (auto *conn : failedConnections)
            {
                connections_.erase(conn);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Finance homeData broadcast failed: " << e.what() << std::endl;
    }
}
