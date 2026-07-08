#include "financeHomeDataBroadcaster.h"

#include <chrono>
#include <iostream>
#include <utility>

#include "../../../controllers/modules/finance/financeHandler.h"
#include "../../../services/auth/AuthSessionStore.h"
#include "../../../services/rbac/RbacService.h"
#include "../../../utils/Utils.h"
#include "../../../utils/permissions/Permissions.h"
#include "../../redis/RedisClient.h"
#include "../../redis/redisMessageBus/RedisMessageBus.h"

namespace
{
    // 财务首页变更的跨实例广播频道。
    constexpr const char *kFinanceHomeChannel = "realtime:finance-home";
}

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

    // 订阅跨实例广播：注册到统一订阅总线；Redis 未启用时总线不启动，退化为单实例本地通知。
    RedisMessageBus::instance().subscribe(
        kFinanceHomeChannel, [this](const std::string &) { triggerLocalPush(); });
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
void FinanceHomeDataBroadcaster::addConnection(crow::websocket::connection *conn, const ConnectionContext &context)
{
    if (!conn)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[conn] = context;
        pending_update_ = true;
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
        for (const auto &[conn, context] : connections_)
        {
            connections.push_back(conn);
        }
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
// 各自推送给本地连接；单实例或 Redis 不可用时直接触发本地推送。
void FinanceHomeDataBroadcaster::notifyHomeDataChanged()
{
    if (RedisMessageBus::instance().active() && RedisClient::instance().publish(kFinanceHomeChannel, "1"))
    {
        return; // 本实例会通过自身订阅收到消息并触发本地推送
    }
    triggerLocalPush();
}

// 仅唤醒本实例广播线程做一次推送，不再发布，避免订阅回环。
void FinanceHomeDataBroadcaster::triggerLocalPush()
{
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        pending_update_ = true;
    }
    broadcast_cv_.notify_all();
}

// 后台监管循环：没有变化时休眠；多个并发通知会在短暂合并窗口内压缩成一次推送。
void FinanceHomeDataBroadcaster::run()
{
    while (running_)
    {
        std::unique_lock<std::mutex> waitLock(connections_mutex_);
        broadcast_cv_.wait(waitLock, [this]()
                           { return !running_ || pending_update_; });
        if (!running_)
        {
            break;
        }

        pending_update_ = false;

        // 没有订阅者时不查询数据库，减少无意义的后台开销；下一次打开首页会触发首帧推送。
        if (connections_.empty())
        {
            continue;
        }
        waitLock.unlock();

        // 合并短时间内连续发生的多个业务更新，避免一个接口批量操作触发多次查库推送。
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            pending_update_ = false;
        }

        pushHomeData();
    }
}

// 查询财务端首页数据并发送给当前全部 WebSocket 连接。
void FinanceHomeDataBroadcaster::pushHomeData()
{
    std::vector<std::pair<crow::websocket::connection *, ConnectionContext>> connections;
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
        // 先记录发送失败的连接，循环结束后再统一从连接池移除。
        std::unordered_set<crow::websocket::connection *> failedConnections;
        for (const auto &[conn, context] : connections)
        {
            if (!conn)
            {
                continue;
            }

            if (!AuthSessionStore::isSessionCurrent(context.userId, context.sessionVersion) ||
                !RbacService::userHasPermission(dbManager_, context.userId, Permissions::kPortalFinance))
            {
                try
                {
                    conn->close("access_revoked");
                }
                catch (...)
                {
                }
                failedConnections.insert(conn);
                continue;
            }

            try
            {
                nlohmann::json message = {
                    {"event", "homeData"},
                    {"version", 1},
                    {"sentAt", getCreateTime()},
                    {"data", handler.buildHomeData(context.userId)}};
                const std::string payload = message.dump();
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
