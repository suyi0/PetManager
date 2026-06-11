#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <crow.h>

#include "../../../database/DatabaseManager.h"

class FinanceHomeDataBroadcaster
{
public:
    static FinanceHomeDataBroadcaster &instance();

    void start(std::shared_ptr<DatabaseManagerInterface> dbManager);
    void stop();
    void addConnection(crow::websocket::connection *conn);
    void removeConnection(crow::websocket::connection *conn);
    void closeAllConnections(const std::string &reason);
    void notifyHomeDataChanged();

private:
    FinanceHomeDataBroadcaster() = default;

    void run();
    void pushHomeData();

    std::shared_ptr<DatabaseManagerInterface> dbManager_;
    std::thread broadcast_thread_;
    std::atomic<bool> running_{false};
    bool pending_update_{false};
    std::mutex connections_mutex_;
    std::condition_variable broadcast_cv_;
    std::unordered_set<crow::websocket::connection *> connections_;
};
