#pragma once
#include <mysqlx/xdevapi.h>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <regex>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <ctime>
#include <sys/stat.h> // 用于创建目录

#include "../../include/Shared/Utils/Utils.h"
#include "../../include/UserSide/Auth/Verification/Verification.h"
#include "../../include/UserSide/UserRoutes/UserRoutes.h"
#include "../../include/UserSide/ReservationRoutes/ReservationRoutes.h"
#include "../DatabaseManager/DatabaseManager.h"
#include "../../include/UserSide/OrderRoutes/OrderRoutes.h"

class WebSocketServer
{
public:
    // 开始入口
    void start();

    // 定时任务
    void startCodeCleanupTask();

    // 优雅关闭
    void gracefulShutdown();

    // 单例模式
    static WebSocketServer &instance();

    // 提供公共方法访问 signal_received
    bool isSignalReceived() const;

    // 设置app指针
    void setApp(CrowApp* app_ptr) { app_ptr_ = app_ptr; }
    
    // 获取app指针
    crow::App<CorsMiddleware, RateLimitMiddleware>* getApp() { return app_ptr_; }

private:
    // 私有构造函数，防止外部实例化
    // 设置路由
    void setupRoutes();

    // 心跳机制
    void startHeartbeat();

    // 信号处理
    void setupSignalHandlers();

    std::thread cleanup_thread;
    crow::App<CorsMiddleware, RateLimitMiddleware>* app_ptr_ = nullptr;  // 使用指针，以便外部传入
    std::thread server_thread;
    std::atomic<bool> heartbeat_running{true};
    std::thread heartbeat_thread;
    std::unordered_set<crow::websocket::connection *> active_connections;
    std::mutex conn_mutex;
    std::condition_variable shutdown_cv;
    std::atomic<bool> running{true};
    std::atomic<bool> stop_requested{false};
    std::atomic<bool> signal_received{false};
};