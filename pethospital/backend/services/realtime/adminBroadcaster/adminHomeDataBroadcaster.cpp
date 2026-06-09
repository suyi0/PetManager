#include "adminHomeDataBroadcaster.h"

#include <chrono>
#include <iostream>
#include <utility>

#include "../../../controllers/modules/admin/adminHandler.h"
#include "../../../utils/Utils.h"

AdminHomeDataBroadcaster &AdminHomeDataBroadcaster::instance()
{
    static AdminHomeDataBroadcaster instance;
    return instance;
}

void AdminHomeDataBroadcaster::start(std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    dbManager_ = std::move(dbManager);

    if (running_.exchange(true))
    {
        return;
    }

    broadcast_thread_ = std::thread([this]()
                                    { run(); });
}

void AdminHomeDataBroadcaster::stop()
{
    running_ = false;
    broadcast_cv_.notify_all();

    if (broadcast_thread_.joinable())
    {
        broadcast_thread_.join();
    }
}

void AdminHomeDataBroadcaster::addConnection(crow::websocket::connection *conn)
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

void AdminHomeDataBroadcaster::removeConnection(crow::websocket::connection *conn)
{
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(conn);
}

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

void AdminHomeDataBroadcaster::run()
{
    while (running_)
    {
        std::unique_lock<std::mutex> waitLock(connections_mutex_);
        broadcast_cv_.wait_for(waitLock, std::chrono::seconds(5));
        if (!running_)
        {
            break;
        }

        if (connections_.empty())
        {
            continue;
        }
        waitLock.unlock();

        pushHomeData();
    }
}

void AdminHomeDataBroadcaster::pushHomeData()
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
        adminHandler handler(dbManager_);
        nlohmann::json message = {
            {"event", "homeData"},
            {"version", 1},
            {"sentAt", getCreateTime()},
            {"data", handler.buildHomeData()}};
        const std::string payload = message.dump();

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
                std::cerr << "Admin homeData WebSocket send failed: " << e.what() << std::endl;
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
        std::cerr << "Admin homeData broadcast failed: " << e.what() << std::endl;
    }
}
