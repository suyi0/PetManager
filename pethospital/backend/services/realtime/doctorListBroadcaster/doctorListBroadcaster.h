#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

#include <crow.h>

// 用户端预约医生列表实时广播：医生上下班后发一个刷新信号，前端重拉预约医生列表。
// 不带医生数据，避免 WS 侧复制 HTTP 列表查询和权限响应逻辑。
class DoctorListBroadcaster
{
public:
    static DoctorListBroadcaster &instance();

    void start();
    void stop();
    void addConnection(crow::websocket::connection *conn);
    void removeConnection(crow::websocket::connection *conn);
    void closeAllConnections(const std::string &reason);
    void notifyDoctorListChanged();

private:
    DoctorListBroadcaster() = default;

    void run();
    void pushDoctorListSignal();
    void triggerLocalDoctorListChanged();

    std::thread broadcast_thread_;
    std::atomic<bool> running_{false};
    std::mutex connections_mutex_;
    std::condition_variable broadcast_cv_;
    std::unordered_set<crow::websocket::connection *> connections_;
    bool pending_ = false;
};
