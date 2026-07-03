#include "setRoutes.h"
#include "../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../services/realtime/doctorBroadcaster/doctorQueueBroadcaster.h"
#include "../../services/realtime/doctorListBroadcaster/doctorListBroadcaster.h"
#include "../../services/realtime/medicineBroadcaster/medicineStockBroadcaster.h"
#include "../../services/realtime/financeBroadcaster/financeHomeDataBroadcaster.h"
#include "../../services/redis/RedisClient.h"
#include "../../utils/staticFileHandler.h"
#include "../../utils/Utils.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <cstdlib>
#include <filesystem>

namespace
{
std::filesystem::path getFrontendDistPath()
{
    const char *configuredPath = std::getenv("PETMANAGER_FRONTEND_DIST");
    if (configuredPath && configuredPath[0] != '\0')
    {
        return configuredPath;
    }

    return std::filesystem::current_path() / "pethospital" / "frontend" / "dist";
}

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

    const int serverPort = getServerPort();
    if (!isPortAvailable(serverPort))
    {
        throw std::runtime_error("Port " + std::to_string(serverPort) + " is already in use");
    }

    setupRoutes();          // 设置路由
    setupSignalHandlers();  // 设置信号处理
    startCodeCleanupTask(); // 启动定时任务
    AdminHomeDataBroadcaster::instance().start(DatabaseManager::getInstance()); // 启动超级管理员首页实时广播任务
    FinanceHomeDataBroadcaster::instance().start(DatabaseManager::getInstance()); // 启动财务端首页实时广播任务
    DoctorQueueBroadcaster::instance().start(DatabaseManager::getInstance()); // 启动医生端待接诊队列实时广播任务
    MedicineStockBroadcaster::instance().start(); // 启动药品库存实时广播任务
    DoctorListBroadcaster::instance().start(); // 启动用户端预约医生列表实时广播任务
    shutdown_requested = false;
    server_stopped = false;

    server_thread = std::thread([this, serverPort]
                                {
            try {
                app_ptr_->port(serverPort).multithreaded().run();
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
    AdminHomeDataBroadcaster::instance().stop();
    AdminHomeDataBroadcaster::instance().closeAllConnections("server_shutdown");
    FinanceHomeDataBroadcaster::instance().stop();
    FinanceHomeDataBroadcaster::instance().closeAllConnections("server_shutdown");
    DoctorQueueBroadcaster::instance().stop();
    DoctorQueueBroadcaster::instance().closeAllConnections("server_shutdown");
    MedicineStockBroadcaster::instance().stop();
    MedicineStockBroadcaster::instance().closeAllConnections("server_shutdown");
    DoctorListBroadcaster::instance().stop();
    DoctorListBroadcaster::instance().closeAllConnections("server_shutdown");

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

    // 注册仓管路由
    warehouseManagerRoutes::setupwarehouseManagerRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册医生路由
    DoctorRoutes::setupDoctorRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册管理员路由
    adminRoutes::setupAdminRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册人事路由
    personnelRoutes::setupPersonnelRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 注册总裁端路由
    bossRoutes::setupBossRoutes(*app_ptr_, DatabaseManager::getInstance());

    // 健康检查（LB/运维探活）。DB 不可用 → 503；
    // Redis 不可用只降级不失败（全站 Redis 路径均有回退），报 degraded 供告警。
    CROW_ROUTE((*app_ptr_), "/health")
    ([]() {
        bool dbUp = false;
        try
        {
            auto dbManager = DatabaseManager::getInstance();
            auto *session = dbManager ? dbManager->getSession() : nullptr;
            if (session)
            {
                session->sql("SELECT 1").execute();
                dbUp = true;
            }
        }
        catch (const std::exception &)
        {
            dbUp = false;
        }

        auto &redis = RedisClient::instance();
        const std::string redisState = !redis.enabled() ? "disabled"
                                                        : (redis.ping() ? "up" : "down");

        crow::json::wvalue body;
        body["status"] = !dbUp ? "unavailable" : (redisState == "down" ? "degraded" : "ok");
        body["db"] = dbUp ? "up" : "down";
        body["redis"] = redisState;
        return crow::response(dbUp ? 200 : 503, body);
    });

    const auto frontendDistPath = getFrontendDistPath();
    CROW_ROUTE((*app_ptr_), "/")
    ([frontendDistPath](const crow::request &req) {
        return StaticFileHandler::handleRequest(req, frontendDistPath);
    });

    app_ptr_->catchall_route()
    ([frontendDistPath](const crow::request &req) {
        return StaticFileHandler::handleRequest(req, frontendDistPath);
    });
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
