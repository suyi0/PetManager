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

#include "../../utils/Utils.h"
#include "../../controllers/auth/Verification/Verification.h"
#include "../user/UserRoutes.h"
#include "../doctorRoutes/doctorRoutes.h"
#include "../warehouseManagerRoutes/warehouseManagerRoutes.h"
#include "../authRoutes/authRoutes.h"
#include "../adminRoutes/adminRoutes.h"
#include "../personnelRoutes/personnelRoutes.h"
#include "../bossRoutes/bossRoutes.h"

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

    // 是否已收到关闭请求
    bool isShutdownRequested() const;

    // 是否已完成服务线程关闭
    bool isServerStopped() const;

    // 设置app指针
    void setApp(CrowApp* app_ptr) { app_ptr_ = app_ptr; }
    
    // 获取app指针
    crow::App<CorsMiddleware, RateLimitMiddleware>* getApp() { return app_ptr_; }

private:
    // 私有构造函数，防止外部实例化
    // 设置路由
    void setupRoutes();

    // 信号处理
    void setupSignalHandlers();
    void stopCodeCleanupTask();

    std::thread cleanup_thread;
    std::atomic<bool> cleanup_running{false};
    std::mutex cleanup_mutex;                       // 用于保护清理任务
    std::condition_variable cleanup_cv;              // 用于等待清理任务结束
    std::condition_variable shutdown_cv;              // 用于等待服务线程结束
    crow::App<CorsMiddleware, RateLimitMiddleware>* app_ptr_ = nullptr;  // 使用指针，以便外部传入
    std::thread server_thread;
    std::unordered_set<crow::websocket::connection *> active_connections;
    std::mutex conn_mutex;
    std::atomic<bool> shutdown_requested{false};    // 表示“已经收到关闭请求” 在 Ctrl+C 的信号处理里设置
    std::atomic<bool> server_stopped{false};        // 表示“Crow 的 run() 已经真正返回” 在服务线程退出时设置
};
