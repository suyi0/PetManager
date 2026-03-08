#include "./routes/setRoutes/setRoutes.h"
#include "../utils/ScheduledTaskManager/ScheduledTaskManager.h"

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

    // 初始化数据库 - 使用新版API
    DatabaseManager::getInstance()->create_Tables();

    // 初始化定时任务管理器
    auto taskManager = ScheduledTaskManager::getInstance();
    taskManager->initialize(DatabaseManager::getInstance());
    taskManager->start();

    // 使用WebSocketServer的setupRoutes方法注册所有路由
    WebSocketServer::instance().setApp(&app); // 假设你有一个方法来设置app
    
    // 启动服务器（包括HTTP服务和WebSocket服务）
    WebSocketServer::instance().start();

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Server started on port 8081" << std::endl;

    // 主线程循环检测信号
    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl; // 添加这行提示信息
    while (!WebSocketServer::instance().isSignalReceived())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 由主线程触发关闭
    std::cout << "Shutting down server..." << std::endl;
    WebSocketServer::instance().gracefulShutdown();

    // 停止定时任务管理器
    taskManager->stop();

    // 关闭HTTP服务器
    app.stop();

    // 释放数据库单例
    DatabaseManager::destroyInstance();

    return 0;
}