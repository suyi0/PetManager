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

class AdminHomeDataBroadcaster
{
public:
    static AdminHomeDataBroadcaster &instance();

    void start(std::shared_ptr<DatabaseManagerInterface> dbManager);
    void stop();
    void addConnection(crow::websocket::connection *conn);
    void removeConnection(crow::websocket::connection *conn);
    void closeAllConnections(const std::string &reason);
    void notifyHomeDataChanged();

private:
    AdminHomeDataBroadcaster() = default;

    void run();
    void pushHomeData();

    std::shared_ptr<DatabaseManagerInterface> dbManager_;
    std::thread broadcast_thread_;  // 广播线程
    std::atomic<bool> running_{false};  // 运行标志符，用于控制广播线程的生命周期 true 表示运行，false 表示停止）。
    bool pending_update_{false};    // 标志符，表示是否有数据更新待推送
    std::mutex connections_mutex_;  // 一个互斥锁，用于保护 connections_ 集合

    // 一个条件变量，通常与 pending_update_ 和 running_ 配合使用。
    // 广播线程可以在没有更新时在此等待，当有新数据或需要停止时被唤醒，避免了忙等待，节省 CPU 资源。
    std::condition_variable broadcast_cv_;
    
    // 一个哈希集合，存储了所有当前活跃的 WebSocket 连接。
    // 广播线程会遍历这个集合并向每个连接发送数据。
    std::unordered_set<crow::websocket::connection *> connections_;
};
