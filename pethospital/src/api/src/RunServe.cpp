#include "../include/ManagerBack.h"

// 主函数
int main(int argc, char *argv[]) {
// 输出当前进程 PID
#ifdef __linux__
    std::cout << "Crow PID: " << getpid() << std::endl;
#elif _WIN32
    std::cout << "Crow PID: " << GetCurrentProcessId() << std::endl;
#elif __APPLE__
    std::cout << "Crow PID: " << getpid() << std::endl;
#endif

    // 启动服务器
    WebSocketServer::instance().start();

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Server started on port 8081" << std::endl;

    // 初始化数据库 - 使用新版API
    try
    {
        // 从环境变量获取数据库连接信息
        const char *db_host = std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "localhost";
        const char *db_user = std::getenv("DB_USER") ? std::getenv("DB_USER") : "root";
        const char *db_pass = std::getenv("DB_PASS") ? std::getenv("DB_PASS") : "yyl030920";
        const char *db_name = std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "pethospital";

        // 首先尝试使用URI方式连接，使用SSL
        std::string uri = "mysqlx://" + std::string(db_user) + ":" + std::string(db_pass) + "@" + std::string(db_host) + ":33060/" + std::string(db_name);

        try
        {
            g_db_session = new mysqlx::Session(uri);
            g_database = new mysqlx::Schema(g_db_session->getSchema(db_name));
            std::cout << "Database connection successful!" << std::endl;
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "URI连接失败，尝试使用SessionOption方式: " << e.what() << std::endl;

            // 使用新的连接方式，不指定SSL模式（使用默认设置）
            g_db_session = new mysqlx::Session(mysqlx::SessionOption::HOST, db_host,
                                               mysqlx::SessionOption::PORT, 33060,
                                               mysqlx::SessionOption::USER, db_user,
                                               mysqlx::SessionOption::PWD, db_pass,
                                               mysqlx::SessionOption::DB, db_name
                                            );
            g_db_session->sql("SET NAMES 'utf8mb4'").execute(); // 设置字符集
            std::cout << "Database connection successful!" << std::endl;
        }
        // 设置字符集为 UTF-8
        try {
            g_db_session->sql("SET NAMES 'utf8mb4'").execute();
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to set character set: " << e.what() << std::endl;
        }
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "连接失败: " << e.what() << std::endl;
        // 即使数据库连接失败，服务器也应该继续运行
        g_db_session = nullptr;
        g_database = nullptr;
    }

    // 主线程循环检测信号
    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl; // 添加这行提示信息
    while (!WebSocketServer::instance().isSignalReceived())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 由主线程触发关闭
    std::cout << "Shutting down server..." << std::endl;
    WebSocketServer::instance().gracefulShutdown();

    // 清理数据库连接
    delete g_database;
    delete g_db_session;

    return 0;
}