#include "medicineStockBroadcaster.h"

#include <chrono>
#include <iostream>
#include <vector>

#include <nlohmann/json.hpp>

#include "../../redis/RedisClient.h"
#include "../../redis/redisMessageBus/RedisMessageBus.h"
#include "../../../utils/Utils.h"

namespace
{
    // 药品库存变更的跨实例广播频道；消息体仅作触发信号（内容无意义）。
    constexpr const char *kMedicineStockChannel = "realtime:medicine-stock";
}

MedicineStockBroadcaster &MedicineStockBroadcaster::instance()
{
    static MedicineStockBroadcaster instance;
    return instance;
}

void MedicineStockBroadcaster::start()
{
    if (running_.exchange(true))
    {
        return;
    }

    broadcast_thread_ = std::thread([this]() { run(); });

    // 跨实例广播：注册到统一订阅总线（任一实例 publish，所有实例收到后触发本地推送）。
    // Redis 未启用时总线不启动（active()=false），notify 退化为单实例本地通知。
    RedisMessageBus::instance().subscribe(
        kMedicineStockChannel, [this](const std::string &) { triggerLocalStockChanged(); });
}

void MedicineStockBroadcaster::stop()
{
    running_ = false;
    broadcast_cv_.notify_all();

    if (broadcast_thread_.joinable())
    {
        broadcast_thread_.join();
    }
}

void MedicineStockBroadcaster::addConnection(crow::websocket::connection *conn)
{
    if (!conn)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.insert(conn);
}

void MedicineStockBroadcaster::removeConnection(crow::websocket::connection *conn)
{
    if (!conn)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(conn);
}

void MedicineStockBroadcaster::closeAllConnections(const std::string &reason)
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
            // 关停期忽略关闭错误。
        }
    }
}

void MedicineStockBroadcaster::notifyMedicineStockChanged()
{
    // 多实例下先发布到频道，所有实例各自推送本地连接；单实例或 Redis 不可用时直接本地触发。
    if (RedisMessageBus::instance().active() && RedisClient::instance().publish(kMedicineStockChannel, "1"))
    {
        return; // 本实例会通过自身订阅收到并触发本地推送
    }
    triggerLocalStockChanged();
}

void MedicineStockBroadcaster::triggerLocalStockChanged()
{
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        pending_ = true;
    }
    broadcast_cv_.notify_all();
}

void MedicineStockBroadcaster::run()
{
    while (running_)
    {
        {
            std::unique_lock<std::mutex> lock(connections_mutex_);
            broadcast_cv_.wait(lock, [this]() { return !running_ || pending_; });

            if (!running_)
            {
                break;
            }
            pending_ = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 去抖：合并突发的多次库存变更
        pushStockSignal();
    }
}

void MedicineStockBroadcaster::pushStockSignal()
{
    std::vector<crow::websocket::connection *> connections;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections.assign(connections_.begin(), connections_.end());
    }

    if (connections.empty())
    {
        return;
    }

    // 仅信号，不带数据：前端收到后自行重拉药品列表（命中失效后的新库存）。
    nlohmann::json message = {
        {"event", "medicineStock"},
        {"version", 1},
        {"sentAt", getCreateTime()}};
    const std::string payload = message.dump();

    std::vector<crow::websocket::connection *> failedConnections;
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
            std::cerr << "Medicine stock WebSocket send failed: " << e.what() << std::endl;
            failedConnections.push_back(conn);
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
