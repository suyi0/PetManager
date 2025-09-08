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

#include "Verification.h"
#include "User.h"
#include "Encrypt.h"
#include "GetAddress.h"
#include "Utils.h"

// 添加全局数据库会话变量的外部声明
extern mysqlx::Session *g_db_session;
extern mysqlx::Schema *g_database;

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

private:
    // 私有构造函数，防止外部实例化

    // 设置路由
    void setupRoutes();

    // 心跳机制
    void startHeartbeat();

    // 信号处理
    void setupSignalHandlers();

    std::thread cleanup_thread;
    crow::SimpleApp app;
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