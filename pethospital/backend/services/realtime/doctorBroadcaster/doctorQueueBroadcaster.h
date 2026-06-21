#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <crow.h>

#include "../../../database/DatabaseManager.h"

class DoctorQueueBroadcaster
{
public:
    static DoctorQueueBroadcaster &instance();

    void start(std::shared_ptr<DatabaseManagerInterface> dbManager);
    void stop();
    void addConnection(crow::websocket::connection *conn, int doctorId);
    void removeConnection(crow::websocket::connection *conn);
    void closeAllConnections(const std::string &reason);
    void notifyQueueChanged(int doctorId);

private:
    DoctorQueueBroadcaster() = default;

    void run();
    void pushQueueData(int doctorId);

    std::shared_ptr<DatabaseManagerInterface> dbManager_;
    std::thread broadcast_thread_;
    std::atomic<bool> running_{false};
    std::mutex connections_mutex_;
    std::condition_variable broadcast_cv_;
    std::unordered_map<int, std::unordered_set<crow::websocket::connection *>> connections_by_doctor_;
    std::unordered_map<crow::websocket::connection *, int> doctor_by_connection_;
    std::unordered_set<int> pending_doctor_updates_;
};
