#include "./routes/setRoutes/setRoutes.h"
#include "../utils/scheduledTaskManager/scheduledTaskManager.h"
#ifndef _WIN32
#include <csignal>
#include <pthread.h>
#endif

// 主函数
int main(int argc, char *argv[])
{
    // 初始化环境变量
    initializeEnvironment();

// 输出当前进程 PID
#ifdef __linux__
    std::cout << "Crow PID: " << getpid() << std::endl;
#elif _WIN32
    std::cout << "Crow PID: " << GetCurrentProcessId() << std::endl;
#elif __APPLE__
    std::cout << "Crow PID: " << getpid() << std::endl;
#endif

    crow::App<CorsMiddleware, RateLimitMiddleware> app;
    
    RateLimitMiddleware::initialize(50, 60);  // 每60s，只能接受50次请求

    // 初始化数据库 - 数据库是核心依赖，连接失败时直接退出，避免后续空会话崩溃
    auto dbManager = DatabaseManager::getInstance();
    if (!dbManager || !dbManager->getSession() || !dbManager->getSchema())
    {
        std::cerr << "Database unavailable. Server startup aborted." << std::endl;
        DatabaseManager::destroyInstance();
        return 1;
    }

    // 初始化定时任务管理器
    auto taskManager = ScheduledTaskManager::getInstance();
    taskManager->initialize(dbManager);

    try
    {
        // 使用WebSocketServer的setupRoutes方法注册所有路由
        WebSocketServer::instance().setApp(&app); // 假设你有一个方法来设置app

        // 启动服务器（包括HTTP服务和WebSocket服务）
        WebSocketServer::instance().start();
        taskManager->start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server startup aborted: " << e.what() << std::endl;
        taskManager->stop();
        DatabaseManager::destroyInstance();
        return 1;
    }

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Server started on port 8081" << std::endl;

    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
    while (!WebSocketServer::instance().isShutdownRequested() &&
           !WebSocketServer::instance().isServerStopped())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!WebSocketServer::instance().isServerStopped())
    {
        // 由主线程触发关闭
        std::cout << "Shutting down server..." << std::endl;
        WebSocketServer::instance().gracefulShutdown();
    }

    // 停止定时任务管理器
    taskManager->stop();

    // 释放数据库单例
    DatabaseManager::destroyInstance();

    return 0;
}
