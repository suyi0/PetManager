#include "doctorListBroadcaster.h"

#include <chrono>
#include <iostream>
#include <vector>

#include <nlohmann/json.hpp>

#include "../../redis/RedisClient.h"
#include "../../../utils/Utils.h"

namespace
{
    constexpr const char *kDoctorListChannel = "realtime:doctor-list";
}

DoctorListBroadcaster &DoctorListBroadcaster::instance()
{
    static DoctorListBroadcaster instance;
    return instance;
}

void DoctorListBroadcaster::start()
{
    if (running_.exchange(true))
    {
        return;
    }

    broadcast_thread_ = std::thread([this]() { run(); });
    subscription_ = RedisClient::instance().subscribe(
        kDoctorListChannel, [this](const std::string &) { triggerLocalDoctorListChanged(); });
}

void DoctorListBroadcaster::stop()
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

void DoctorListBroadcaster::addConnection(crow::websocket::connection *conn)
{
    if (!conn)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.insert(conn);
}

void DoctorListBroadcaster::removeConnection(crow::websocket::connection *conn)
{
    if (!conn)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(conn);
}

void DoctorListBroadcaster::closeAllConnections(const std::string &reason)
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
        }
    }
}

void DoctorListBroadcaster::notifyDoctorListChanged()
{
    if (subscription_ && RedisClient::instance().publish(kDoctorListChannel, "1"))
    {
        return;
    }
    triggerLocalDoctorListChanged();
}

void DoctorListBroadcaster::triggerLocalDoctorListChanged()
{
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        pending_ = true;
    }
    broadcast_cv_.notify_all();
}

void DoctorListBroadcaster::run()
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

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        pushDoctorListSignal();
    }
}

void DoctorListBroadcaster::pushDoctorListSignal()
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

    nlohmann::json message = {
        {"event", "doctorList"},
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
            std::cerr << "Doctor list WebSocket send failed: " << e.what() << std::endl;
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
