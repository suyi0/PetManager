#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <tuple>

#include "DatabaseManager.h"
#include "DatabaseMigrations.h"

namespace
{
    // 缓存连接的活性检测节流间隔：间隔内直接复用（零额外往返），
    // 超过间隔的下一次 getSession 先 ping 确认。取 5s 在"MySQL 重启后
    // 最多 5s 内自愈"与"高频请求零 ping 开销"之间平衡。
    constexpr std::chrono::seconds kSessionValidationInterval{5};

    // 清理 .env 里可能带上的引号，避免配置值解析异常。
    std::string trimQuotes(std::string value)
    {
        if (value.size() >= 2)
        {
            const char first = value.front();
            const char last = value.back();
            if ((first == '"' && last == '"') || (first == '\'' && last == '\''))
            {
                return value.substr(1, value.size() - 2);   // 从第二个字符符开始截取，长度减少2（把尾巴的引号去掉)
            }
        }

        return value;
    }

    // 统一解析布尔型环境变量，兼容 1/true/yes/on 等写法。
    bool isTruthyEnv(const char *value)
    {
        if (!value)
        {
            return false;
        }

        std::string normalized = trimQuotes(std::string(value));
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch)
                        {
                            return static_cast<char>(std::toupper(ch));
                        });
        return normalized == "1" || normalized == "TRUE" || normalized == "YES" || normalized == "ON";
    }

    // 判断是否需要运行数据库迁移和创建数据库表
    bool shouldRunStartupMigrations()
    {
        return isTruthyEnv(std::getenv("DB_AUTO_RUN_MIGRATIONS"));
    }

    // 每个 MySQL session 都固定为 UTC，避免 TIMESTAMP 字段受数据库默认时区影响。
    void configureSessionTimeZone(mysqlx::Session &session)
    {
        session.sql("SET SESSION time_zone = '+00:00'").execute();
    }

    // 获取数据库SSL模式
    mysqlx::SSLMode parseSslMode(const char *ssl_mode_env)
    {
        if (!ssl_mode_env)
        {
            return mysqlx::SSLMode::REQUIRED;
        }

        std::string ssl_mode = trimQuotes(ssl_mode_env);
        std::transform(ssl_mode.begin(), ssl_mode.end(), ssl_mode.begin(), [](unsigned char ch)
                       { return static_cast<char>(std::toupper(ch)); });

        if (ssl_mode == "DISABLED")
        {
            if (!isTruthyEnv(std::getenv("DB_ALLOW_INSECURE_SSL")))
            {
                std::cerr << "⚠️  Refusing DB_SSL_MODE=DISABLED without DB_ALLOW_INSECURE_SSL=true. Falling back to REQUIRED."
                          << std::endl;
                return mysqlx::SSLMode::REQUIRED;
            }
            return mysqlx::SSLMode::DISABLED;
        }
        if (ssl_mode == "VERIFY_CA")
        {
            return mysqlx::SSLMode::VERIFY_CA;
        }
        if (ssl_mode == "VERIFY_IDENTITY")
        {
            return mysqlx::SSLMode::VERIFY_IDENTITY;
        }
        if (ssl_mode == "REQUIRED")
        {
            return mysqlx::SSLMode::REQUIRED;
        }

        std::cerr << "⚠️  Unknown DB_SSL_MODE value '" << ssl_mode
                  << "', falling back to REQUIRED" << std::endl;
        return mysqlx::SSLMode::REQUIRED;
    }

    // 将SSL模式转换为字符串
    const char *sslModeToString(mysqlx::SSLMode mode)
    {
        switch (mode)
        {
        case mysqlx::SSLMode::DISABLED:
            return "DISABLED";
        case mysqlx::SSLMode::REQUIRED:
            return "REQUIRED";
        case mysqlx::SSLMode::VERIFY_CA:
            return "VERIFY_CA";
        case mysqlx::SSLMode::VERIFY_IDENTITY:
            return "VERIFY_IDENTITY";
        default:
            return "UNKNOWN";
        }
    }
}

// 从环境变量中加载数据库基础连接配置。
std::tuple<std::string, int, std::string, std::string, std::string> loadDatabaseConfig()
{
    std::string host;
    int port = 0;
    std::string user;
    std::string password;
    std::string name;

    const char *db_host = std::getenv("DB_HOST");
    const char *db_port = std::getenv("DB_PORT");
    const char *db_user = std::getenv("DB_USER");
    const char *db_pass = std::getenv("DB_PASS");
    const char *db_name = std::getenv("DB_NAME");
    if (db_host && db_port && db_user && db_pass && db_name)
    {
        host = trimQuotes(std::string(db_host));
        try
        {
            port = std::stoi(std::string(db_port)); // 将字符串转换为整数
        }
        catch (const std::exception &)
        {
            throw std::runtime_error("Invalid DB_PORT value: " + std::string(db_port));
        }
        user = trimQuotes(std::string(db_user));
        password = trimQuotes(std::string(db_pass));
        name = trimQuotes(std::string(db_name));
    }

    if (host.empty() || port <= 0 || user.empty() || password.empty() || name.empty())
    {
        throw std::runtime_error("Missing required database environment variables: DB_HOST, DB_PORT, DB_USER, DB_PASS, DB_NAME");
    }

    return std::make_tuple(host, port, user, password, name);
}

std::mutex DatabaseManager::mutex_;
thread_local std::unique_ptr<mysqlx::Session> DatabaseManager::thread_session_ = nullptr;
thread_local std::unique_ptr<mysqlx::Schema> DatabaseManager::thread_schema_ = nullptr;
thread_local std::chrono::steady_clock::time_point DatabaseManager::thread_validated_at_{};
thread_local bool DatabaseManager::thread_session_used_ = false;
std::shared_ptr<DatabaseManagerInterface> DatabaseManager::instance = nullptr;

// 当前线程是否已经持有 session/schema 对象（不代表连接仍然存活，
// 活性由 ensureThreadConnection 的节流 ping 判断）。
bool DatabaseManager::hasValidConnection() const
{
    return thread_session_ != nullptr && thread_schema_ != nullptr;
}

// 对外保留原有入口，但实际迁移逻辑已经拆到独立模块。
void DatabaseManager::create_Tables()
{
    DatabaseMigrations::run(*this);
}

// 构造阶段只负责建立连接，并按开关决定是否在启动时执行迁移。
DatabaseManager::DatabaseManager()
{
    try
    {
        auto [db_host, db_port, db_user, db_pass, db_name] = loadDatabaseConfig();
        db_host_ = db_host;
        db_port_ = db_port;
        db_user_ = db_user;
        db_pass_ = db_pass;
        db_name_ = db_name;
        ssl_mode_ = parseSslMode(std::getenv("DB_SSL_MODE"));

        if (ensureThreadConnection())
        {
            if (shouldRunStartupMigrations())
            {
                create_Tables();
            }
            else
            {
                std::cout << "Skipping startup database DDL. Set DB_AUTO_RUN_MIGRATIONS=true to enable." << std::endl;
            }
        }
    }
    catch (const std::exception &e)
    {
        // 注意：这里不能直接把 std::getenv 结果塞进流（缺失时为 nullptr，UB）；
        // 缺哪些变量 loadDatabaseConfig 的异常信息里已经写明。
        std::cerr << "❌ 数据库连接发生异常: " << e.what() << std::endl;
        thread_session_.reset();
        thread_schema_.reset();
    }
}

// 建立一条已选中目标库、UTC 时区的新连接；失败抛 mysqlx::Error。
// 30s 连接超时沿用旧 URI 路径的 connect-timeout=30000 语义。
std::unique_ptr<mysqlx::Session> DatabaseManager::openSessionWithDatabase() const
{
    auto session = std::make_unique<mysqlx::Session>(
        mysqlx::SessionOption::HOST, db_host_.c_str(),
        mysqlx::SessionOption::PORT, db_port_,
        mysqlx::SessionOption::USER, db_user_.c_str(),
        mysqlx::SessionOption::PWD, db_pass_.c_str(),
        mysqlx::SessionOption::DB, db_name_.c_str(),
        mysqlx::SessionOption::SSL_MODE, ssl_mode_,
        mysqlx::SessionOption::CONNECT_TIMEOUT, 30000);
    configureSessionTimeZone(*session);
    return session;
}

void DatabaseManager::adoptThreadSession(std::unique_ptr<mysqlx::Session> session)
{
    auto schema = std::make_unique<mysqlx::Schema>(session->getSchema(db_name_));
    // 作用是把 session 和 schema 绑定到当前线程的 thread_local 变量中，确保每个线程都有独立的数据库连接和模式对象。
    thread_session_ = std::move(session);
    thread_schema_ = std::move(schema);
    thread_validated_at_ = std::chrono::steady_clock::now();    // 更新最后验证时间
}

// 为每个线程建立独立数据库连接，避免跨线程复用同一个 MySQL X session。
// 已有连接会按节流间隔 ping 活性，死连接（MySQL 重启 / wait_timeout）丢弃重建。
bool DatabaseManager::ensureThreadConnection()
{
    if (thread_session_ && thread_schema_)  // 已有连接，检查活性
    {
        const auto now = std::chrono::steady_clock::now();
        if (now - thread_validated_at_ < kSessionValidationInterval)
        {
            return true;
        }
        try
        {
            thread_session_->sql("SELECT 1").execute();
            thread_validated_at_ = now;
            return true;
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "⚠️  Cached DB session is dead (" << e.what()
                      << "), reconnecting..." << std::endl;
            thread_schema_.reset();
            thread_session_.reset();
        }
    }

    // 没有连接或连接已死：尝试建立新连接。

    // 唯一连接路径：SessionOption 直连目标库。
    // （旧代码的 URI / host-string 回退与它走的是同一条协议路径，属于冗余，已移除。）
    try
    {
        adoptThreadSession(openSessionWithDatabase());
        std::cout << "✅ Database connection established. SSL mode: "
                  << sslModeToString(ssl_mode_) << std::endl;
        return true;
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "❌ Database connection failed: " << e.what() << std::endl;
    }

    // 直连失败通常是目标库不存在（全新环境）：仅在允许启动迁移时，
    // 连接到服务器级按需建库后重试一次；禁用迁移时不做任何隐式 DDL。
    if (!shouldRunStartupMigrations())
    {
        std::cerr << "Skipping automatic database creation because DB_AUTO_RUN_MIGRATIONS is disabled." << std::endl;
        return false;
    }
    try
    {
        mysqlx::Session server_session(
            mysqlx::SessionOption::HOST, db_host_.c_str(),
            mysqlx::SessionOption::PORT, db_port_,
            mysqlx::SessionOption::USER, db_user_.c_str(),
            mysqlx::SessionOption::PWD, db_pass_.c_str(),
            mysqlx::SessionOption::SSL_MODE, ssl_mode_,
            mysqlx::SessionOption::CONNECT_TIMEOUT, 30000);
        server_session
            .sql("CREATE DATABASE IF NOT EXISTS `" + db_name_ + "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
            .execute();
        std::cout << "Database '" << db_name_ << "' ensured, reconnecting..." << std::endl;

        adoptThreadSession(openSessionWithDatabase());
        std::cout << "✅ Database connection established after creating schema. SSL mode: "
                  << sslModeToString(ssl_mode_) << std::endl;
        return true;
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "❌ Database connection failed (after create-database attempt): " << e.what() << std::endl;
        std::cerr << "连接参数详情:" << std::endl;
        std::cerr << "- Host: " << db_host_ << std::endl;
        std::cerr << "- Port: " << db_port_ << std::endl;
        std::cerr << "- User: " << db_user_ << std::endl;
        std::cerr << "- DB Name: " << db_name_ << std::endl;
        std::cerr << "- SSL Mode: " << sslModeToString(ssl_mode_) << std::endl;
        thread_session_.reset();
        thread_schema_.reset();
        return false;
    }
}

std::shared_ptr<DatabaseManagerInterface> DatabaseManager::getInstance()
{
    // 单例初始化加锁，确保多线程下只创建一个 DatabaseManager。
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance == nullptr)
    {
        instance = std::shared_ptr<DatabaseManagerInterface>(new DatabaseManager());
    }
    return instance;
}

// 显式释放单例
void DatabaseManager::destroyInstance()
{
    // 显式释放单例，便于服务关闭时清理数据库资源。
    std::lock_guard<std::mutex> lock(mutex_);
    instance.reset();
}

void DatabaseManager::endOfRequestCleanup()
{
    // 没把会话交给过业务代码的请求直接返回，零开销。
    if (!thread_session_used_)
    {
        return;
    }
    thread_session_used_ = false;
    if (!thread_session_)
    {
        return;
    }
    try
    {
        // 防御性回滚：handler 带着未完成事务返回（异常 / 漏 commit）时，
        // 掐断对同线程下一个请求的污染；无事务时 ROLLBACK 是服务端 no-op。
        thread_session_->sql("ROLLBACK").execute();
    }
    catch (const mysqlx::Error &)
    {
        // 连 ROLLBACK 都失败说明连接状态不明：直接丢弃，下次 getSession 重建。
        thread_schema_.reset();
        thread_session_.reset();
    }
}

// 获取当前线程的数据库会话对象，如果没有有效连接则返回 nullptr。
mysqlx::Session *DatabaseManager::getSession()
{
    // 按需确保当前线程连接存在（含活性检测），再返回底层 session。
    if (!ensureThreadConnection())
    {
        return nullptr;
    }
    thread_session_used_ = true;
    return thread_session_.get();
}

// 获取当前线程的数据库模式对象，如果没有有效连接则返回 nullptr。
mysqlx::Schema *DatabaseManager::getSchema()
{
    // schema 依赖 session，同样走线程级懒初始化。
    if (!ensureThreadConnection())
    {
        return nullptr;
    }
    thread_session_used_ = true;
    return thread_schema_.get();
}

DatabaseManager::~DatabaseManager()
{
    // 当前线程退出前释放持有的数据库连接对象。
    thread_session_.reset();
    thread_schema_.reset();
}
