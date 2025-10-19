#include "../include/ManagerBack.h"

// 在ManagerBack.cpp中添加一个重置自增计数器的函数
void resetAutoIncrement(const std::string &table_name)
{
    try
    {
        // 使用TRUNCATE TABLE来清空表并重置自增计数器
        std::string sql = "TRUNCATE TABLE " + table_name;
        g_db_session->sql(sql).execute();

        std::cout << "Table '" << table_name << "' truncated and auto-increment reset." << std::endl;
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "Error truncating table '" << table_name << "': " << e.what() << std::endl;
    }
}
// 在ManagerBack.cpp中添加一个手动设置自增计数器的函数
void setAutoIncrement(const std::string &table_name, int new_value)
{
    try
    {
        // 设置自增计数器的下一个值
        std::string sql = "ALTER TABLE " + table_name + " AUTO_INCREMENT = " + std::to_string(new_value);
        g_db_session->sql(sql).execute();

        std::cout << "Auto-increment for table '" << table_name << "' set to " << new_value << "." << std::endl;
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "Error setting auto-increment for table '" << table_name << "': " << e.what() << std::endl;
    }
}

void WebSocketServer::start()
{
    setupRoutes();          // 设置路由
    setupSignalHandlers();  // 设置信号处理
    startCodeCleanupTask(); // 启动定时任务

    server_thread = std::thread([this]
                                {
            try {
                app.port(8081).multithreaded().run();
            } catch (const std::exception& e) {
                std::cerr << "Server fatal error: " << e.what() << std::endl;
            } });
}

void WebSocketServer::startCodeCleanupTask()
{
    cleanup_thread = std::thread([]()
                                 {
            while (true) {
                // 每隔一段时间清理一次过期验证码
                std::this_thread::sleep_for(std::chrono::minutes(5));
                Verify::CleanupExpiredCodes();
            } });
    cleanup_thread.detach();
}

void WebSocketServer::gracefulShutdown()
{
    heartbeat_running = false;
    if (heartbeat_thread.joinable())
        heartbeat_thread.join();

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
    app.stop();
    if (server_thread.joinable())
    {
        server_thread.join();
    }
    stop_requested = true;
    std::cout << "Server shutdown complete" << std::endl;
}

// 实现WebSocketServer的单例模式instance方法
WebSocketServer &WebSocketServer::instance()
{
    static WebSocketServer instance;
    return instance;
}

// 提供公共方法访问 signal_received
bool WebSocketServer::isSignalReceived() const
{
    return signal_received.load();
    /*load() 方法支持显式指定内存顺序（默认为std::memory_order_seq_cst）。该顺序要求：
    当前线程的后续操作不会重排到load()之前。
    其他线程对同一原子的写入对所有线程可见。
    在信号处理场景中，这确保主线程能立即感知到信号标志的变化，避免因编译器/CPU指令重排导致延迟可见。*/
}

// 设置路由
void WebSocketServer::setupRoutes()
{

    // 注册用户路由
    UserRoutes::instance().setupUserRoutes(app);
    
    // 注册预约路由
    ReservationRoutes::instance().setupReservationRoutes(app);
    
    CROW_WEBSOCKET_ROUTE(app, "/websocket")
        // 连接开启时的onOpen回调
        .onopen([&](crow::websocket::connection &conn)
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
        .onclose([&](crow::websocket::connection &conn, const std::string &reason, uint16_t value)
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
        .onmessage([&](crow::websocket::connection &conn, const std::string &data, bool is_binary)
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
        .onerror([&](crow::websocket::connection &conn, const std::string &reason)
                 { std::cerr << "WebSocket error: " << reason << std::endl; });
}

// 启动心跳线程，定期发送ping消息
void WebSocketServer::startHeartbeat()
{
    heartbeat_thread = std::thread([this]
                                   {
            while (heartbeat_running) {
                std::this_thread::sleep_for(std::chrono::seconds(30));
                std::unordered_set<crow::websocket::connection*> connections_copy;
                // 获取连接副本
                {
                    std::lock_guard<std::mutex> lock(conn_mutex);
                    connections_copy = active_connections;
                }
                // 创建可能需要移除的连接列表
                std::vector<crow::websocket::connection*> to_remove;
                for (auto* conn : connections_copy) {
                    try {
                        if (conn) {
                            conn->send_ping("ping"); // Crow支持ping/pong
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error sending ping: " << e.what() << std::endl;
                        to_remove.push_back(conn);
                    }
                }
                // 移除无效连接
                if (!to_remove.empty()) {
                    std::lock_guard<std::mutex> lock(conn_mutex);
                    for (auto* conn : to_remove) {
                        active_connections.erase(conn);
                    }
                }
            } });
}

// 设置信号处理函数
void WebSocketServer::setupSignalHandlers()
{
    // 使用更安全的 sigaction 替代 signal
    struct sigaction sa;
    sa.sa_handler = [](int)
    { instance().signal_received = true; };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}
