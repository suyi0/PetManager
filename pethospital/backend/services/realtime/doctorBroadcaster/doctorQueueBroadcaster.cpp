#include "doctorQueueBroadcaster.h"

#include <chrono>
#include <iostream>
#include <utility>

#include "../../../controllers/modules/doctor/doctorHandler.h"
#include "../../../utils/Utils.h"

DoctorQueueBroadcaster &DoctorQueueBroadcaster::instance()
{
    static DoctorQueueBroadcaster instance;
    return instance;
}

void DoctorQueueBroadcaster::start(std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    dbManager_ = std::move(dbManager);

    if (running_.exchange(true))
    {
        return;
    }

    broadcast_thread_ = std::thread([this]() { run(); });
}

void DoctorQueueBroadcaster::stop()
{
    running_ = false;
    broadcast_cv_.notify_all();

    if (broadcast_thread_.joinable())
    {
        broadcast_thread_.join();
    }
}

void DoctorQueueBroadcaster::addConnection(crow::websocket::connection *conn, int doctorId)
{
    if (!conn || doctorId <= 0)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_by_doctor_[doctorId].insert(conn);
        doctor_by_connection_[conn] = doctorId;
        pending_doctor_updates_.insert(doctorId);
    }

    broadcast_cv_.notify_all();
}

void DoctorQueueBroadcaster::removeConnection(crow::websocket::connection *conn)
{
    if (!conn)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto doctorIt = doctor_by_connection_.find(conn);
    if (doctorIt == doctor_by_connection_.end())
    {
        return;
    }

    const int doctorId = doctorIt->second;
    doctor_by_connection_.erase(doctorIt);

    auto connectionsIt = connections_by_doctor_.find(doctorId);
    if (connectionsIt != connections_by_doctor_.end())
    {
        connectionsIt->second.erase(conn);
        if (connectionsIt->second.empty())
        {
            connections_by_doctor_.erase(connectionsIt);
            pending_doctor_updates_.erase(doctorId);
        }
    }
}

void DoctorQueueBroadcaster::closeAllConnections(const std::string &reason)
{
    std::vector<crow::websocket::connection *> connections;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections.reserve(doctor_by_connection_.size());
        for (const auto &entry : doctor_by_connection_)
        {
            connections.push_back(entry.first);
        }
        connections_by_doctor_.clear();
        doctor_by_connection_.clear();
        pending_doctor_updates_.clear();
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
            // Ignore close errors during server shutdown.
        }
    }
}

void DoctorQueueBroadcaster::notifyQueueChanged(int doctorId)
{
    if (doctorId <= 0)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        pending_doctor_updates_.insert(doctorId);
    }
    broadcast_cv_.notify_all();
}

void DoctorQueueBroadcaster::run()
{
    while (running_)
    {
        std::vector<int> pendingDoctors;
        {
            std::unique_lock<std::mutex> lock(connections_mutex_);
            broadcast_cv_.wait(lock, [this]() {
                return !running_ || !pending_doctor_updates_.empty();
            });

            if (!running_)
            {
                break;
            }

            pendingDoctors.assign(pending_doctor_updates_.begin(), pending_doctor_updates_.end());
            pending_doctor_updates_.clear();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        for (const int doctorId : pendingDoctors)
        {
            pushQueueData(doctorId);
        }
    }
}

void DoctorQueueBroadcaster::pushQueueData(int doctorId)
{
    std::vector<crow::websocket::connection *> connections;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        auto it = connections_by_doctor_.find(doctorId);
        if (it == connections_by_doctor_.end() || it->second.empty())
        {
            return;
        }
        connections.assign(it->second.begin(), it->second.end());
    }

    if (!dbManager_ || connections.empty())
    {
        return;
    }

    try
    {
        doctorHandler handler(dbManager_);
        nlohmann::json message = {
            {"event", "doctorQueue"},
            {"version", 1},
            {"sentAt", getCreateTime()},
            {"data", handler.buildQueuesData(doctorId)}};
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
                std::cerr << "Doctor queue WebSocket send failed: " << e.what() << std::endl;
                failedConnections.insert(conn);
            }
        }

        if (!failedConnections.empty())
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            for (auto *conn : failedConnections)
            {
                auto doctorIt = doctor_by_connection_.find(conn);
                if (doctorIt == doctor_by_connection_.end())
                {
                    continue;
                }

                const int failedDoctorId = doctorIt->second;
                doctor_by_connection_.erase(doctorIt);
                auto connectionsIt = connections_by_doctor_.find(failedDoctorId);
                if (connectionsIt != connections_by_doctor_.end())
                {
                    connectionsIt->second.erase(conn);
                    if (connectionsIt->second.empty())
                    {
                        connections_by_doctor_.erase(connectionsIt);
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Doctor queue broadcast failed: " << e.what() << std::endl;
    }
}
