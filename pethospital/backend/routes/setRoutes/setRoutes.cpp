#include "setRoutes.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace
{
bool isPortAvailable(int port)
{
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        return false;
    }
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return false;
    }
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    const int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(yes));

    const bool available =
        bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0;

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif

    return available;
}
}

void WebSocketServer::start()
{
    if (!app_ptr_)
    {
        throw std::runtime_error("App pointer is null in WebSocketServer::start");
    }

    if (!isPortAvailable(8081))
    {
        throw std::runtime_error("Port 8081 is already in use");
    }

    setupRoutes();          // 设置路由
    setupSignalHandlers();  // 设置信号处理
    startCodeCleanupTask(); // 启动定时任务
    shutdown_requested = false;
    server_stopped = false;

    server_thread = std::thread([this]
                                {
            try {
                app_ptr_->port(8081).multithreaded().run();
            } catch (const std::exception& e) {
                std::cerr << "Server fatal error: " << e.what() << std::endl;
            }
            server_stopped = true;
            shutdown_cv.notify_all(); });
}

void WebSocketServer::startCodeCleanupTask()
{
    cleanup_running = true;
    cleanup_thread = std::thread([this]()
                                 {
            while (cleanup_running) {
                std::unique_lock<std::mutex> lock(cleanup_mutex);
                if (cleanup_cv.wait_for(lock, std::chrono::minutes(5), [this]() {
                        return !cleanup_running.load();
                    })) {
                    break;
                }
                lock.unlock();
                Verify::CleanupExpiredCodes();
            } });
}

void WebSocketServer::stopCodeCleanupTask()
{
    cleanup_running = false;
    cleanup_cv.notify_all();
    if (cleanup_thread.joinable())
    {
        cleanup_thread.join();
    }
}

void WebSocketServer::gracefulShutdown()
{
    shutdown_requested = true;
    std::cout << "Initiating graceful shutdown..." << std::endl;

    // 关闭所有活跃连接 [1,5](@ref)
    {
        std::lock_guard<std::mutex> lock(conn_mutex);
        // 创建连接副本以避免在迭代时修改集合
        auto connections_copy = active_connections;
        for (auto *conn : connections_copy)
        {
            if (conn)
            {
                try
                {
                    conn->close("server_shutdown");
                }
                catch (...)
                {
                    // 忽略关闭连接时的异常
                }
            }
        }
    }

    // 等待连接关闭（仅检测active_connections为空）
    std::unique_lock<std::mutex> lk(conn_mutex);
    shutdown_cv.wait(lk, [this]
                     {
                         return active_connections.empty(); // 仅等待连接清空
                     });
    // 停止服务器
    app_ptr_->stop();
    if (server_thread.joinable())
    {
        server_thread.join();
    }
    stopCodeCleanupTask();
    server_stopped = true;
    std::cout << "Server shutdown complete" << std::endl;
}

// 实现WebSocketServer的单例模式instance方法
WebSocketServer &WebSocketServer::instance()
{
    static WebSocketServer instance;
    return instance;
}

bool WebSocketServer::isShutdownRequested() const
{
    return shutdown_requested.load();
}

bool WebSocketServer::isServerStopped() const
{
    return server_stopped.load();
}

// 设置路由
void WebSocketServer::setupRoutes()
{
    if (!app_ptr_) {
        std::cerr << "Error: App pointer is null in setupRoutes" << std::endl;
        return;
    }

    // 注册认证路由
    authRoutes::setupAuthRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册用户路由
    UserRoutes::setupUserRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册预约路由
    ReservationRoutes::setupReservationRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册订单路由
    OrderRoutes::setupOrderRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册仓库路由
    warehouseManagerRoutes::setupwarehouseManagerRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册医生路由
    DoctorRoutes::setupDoctorRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册管理员路由
    adminRoutes::setupAdminRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册人事路由
    personnelRoutes::setupPersonnelRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册总裁端路由
    bossRoutes::setupBossRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 使用解引用后的对象注册WebSocket路由
    auto& app_ref = *app_ptr_;
    CROW_WEBSOCKET_ROUTE(app_ref, "/websocket")
        // 连接开启时的onOpen回调
        .onopen([this](crow::websocket::connection &conn)
                {
        std::lock_guard<std::mutex> lock(conn_mutex);
        active_connections.insert(&conn);
        std::cout << "New WebSocket connection opened. Total connections: " << active_connections.size() << std::endl;

        // 发送欢迎消息
        nlohmann::json json_msg = {{"message", "Connected to C++ WebSocket!"}};
        try
        {
            conn.send_text(json_msg.dump());
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error sending welcome message: " << e.what() << std::endl;
        } })
        .onclose([this](crow::websocket::connection &conn, const std::string &reason, uint16_t value)
                 {
                     std::lock_guard<std::mutex> lock(conn_mutex);
                     active_connections.erase(&conn);
                     std::cout << "Connection closed: Code: " << value << ", Reason: " << reason
                               << ", Remaining connections: " << active_connections.size() << std::endl;
                     // 检查关闭代码
                     if (value != 1000)
                     { // 1000是正常关闭代码
                         std::cout << "Abnormal closure detected" << std::endl;
                     }

                     shutdown_cv.notify_all(); // 通知等待的线程
                 })
        .onmessage([this](crow::websocket::connection &conn, const std::string &data, bool is_binary)
                   {
        std::cout << "Message received: " << data << std::endl;
        // 回显消息
        if (!is_binary)
        {
            try
            {
                nlohmann::json response = {{"echo", data}};
                conn.send_text(response.dump());
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error echoing message: " << e.what() << std::endl;
            }
        } })
        .onerror([this](crow::websocket::connection &conn, const std::string &reason)
                 { std::cerr << "WebSocket error: " << reason << std::endl; });
}

// 设置信号处理函数
void WebSocketServer::setupSignalHandlers()
{
    // 使用更安全的 sigaction
    struct sigaction sa;
    sa.sa_handler = [](int) { instance().shutdown_requested = true; };

    sigemptyset(&sa.sa_mask);   // 清空信号掩码

    // SA_RESTART - 让被信号中断的系统调用自动重启
    // SA_NOCLDWAIT - 防止产生僵尸进程
    // SA_NODEFER - 允许在处理信号期间接收同类型的信号
    // SA_SIGINFO - 使用带扩展信息的信号处理函数
    sa.sa_flags = 0;             // 默认标志，设置信号处理标志为0

    // 注册对SIGINT(Ctrl+C)和SIGTERM(终止信号)的处理，当接收到这些信号时执行上面定义的lambda函数
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}
