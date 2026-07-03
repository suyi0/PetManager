#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

#include <crow.h>

// 药品库存实时广播：任一处库存变更（开单扣减、仓库增/改/删）后广播一个"刷新"信号，
// 让停在药品列表/开单页的医生端重新拉取药品列表（拿到失效后的新库存）。
// 全局广播（不分医生、不带数据），镜像项目现有 *Broadcaster 的线程/去抖/Redis pub-sub 范式。
class MedicineStockBroadcaster
{
public:
    static MedicineStockBroadcaster &instance();

    void start();
    void stop();
    void addConnection(crow::websocket::connection *conn);
    void removeConnection(crow::websocket::connection *conn);
    void closeAllConnections(const std::string &reason);
    void notifyMedicineStockChanged();

private:
    MedicineStockBroadcaster() = default;

    void run();
    void pushStockSignal();
    void triggerLocalStockChanged(); // 仅唤醒本实例推送，不再次发布，避免订阅回环

    std::thread broadcast_thread_;
    std::atomic<bool> running_{false};
    std::mutex connections_mutex_;
    std::condition_variable broadcast_cv_;
    std::unordered_set<crow::websocket::connection *> connections_;
    bool pending_ = false;
};
