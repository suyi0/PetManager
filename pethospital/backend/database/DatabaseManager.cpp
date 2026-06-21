#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <tuple>

#include "DatabaseManager.h"
#include "DatabaseMigrations.h"

namespace
{
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
                            // std::toupper() 函数返回一个 int 类型值，
                            //通过 static_cast<char> 将其安全地转换回 char 类型以匹配 std::transform 所需的类型。
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

    // 将SSL模式转换为URI值
    const char *sslModeToUriValue(mysqlx::SSLMode mode)
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
            return "REQUIRED";
        }
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
            port = std::stoi(std::string(db_port));
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
std::shared_ptr<DatabaseManagerInterface> DatabaseManager::instance = nullptr;

// 当前线程是否已经持有可用的 session/schema。
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
        std::cerr << "❌ 数据库连接发生异常: " << e.what() << std::endl;
        std::cerr << "连接参数详情:" << std::endl;
        std::cerr << "- Host: " << std::getenv("DB_HOST") << std::endl;
        std::cerr << "- Port: " << std::getenv("DB_PORT") << std::endl;
        std::cerr << "- User: " << std::getenv("DB_USER") << std::endl;
        std::cerr << "- Password: " << (std::getenv("DB_PASS") ? "***" : "未设置") << std::endl;
        std::cerr << "- DB Name: " << std::getenv("DB_NAME") << std::endl;
        thread_session_.reset();
        thread_schema_.reset();
    }
}

// 为每个线程建立独立数据库连接，避免跨线程复用同一个 MySQL X session。
bool DatabaseManager::ensureThreadConnection()
{
    if (thread_session_ && thread_schema_)
    {
        return true;
    }

    bool connected = false;

    try
    {
        // 优先使用完整 URI 连接，配置最集中，也最容易复用 SSL 参数。
        std::string full_uri =
            "mysqlx://" + db_user_ + ":" + db_pass_ + "@" + db_host_ + ":" +
            std::to_string(db_port_) + "/" + db_name_ +
            "?connect-timeout=30000&ssl-mode=" + sslModeToUriValue(ssl_mode_);
        
        // 使用std::make_unique来创建一个mysqlx::Session对象的独占指针
        auto session = std::make_unique<mysqlx::Session>(full_uri);
        configureSessionTimeZone(*session);
        auto schema = std::make_unique<mysqlx::Schema>(session->getSchema(db_name_));
        thread_session_ = std::move(session);
        thread_schema_ = std::move(schema);
        std::cout << "✅ Database connection successful via URI method! SSL mode: "
                  << sslModeToString(ssl_mode_) << std::endl;
        connected = true;
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "❌ URI connection failed: " << e.what() << std::endl;
    }

    if (!connected)
    {
        try
        {
            // URI 失败时回退到 SessionOption 方式，兼容不同环境下的连接行为。
            auto session = std::make_unique<mysqlx::Session>(
                mysqlx::SessionOption::HOST, db_host_.c_str(),
                mysqlx::SessionOption::PORT, db_port_,
                mysqlx::SessionOption::USER, db_user_.c_str(),
                mysqlx::SessionOption::PWD, db_pass_.c_str(),
                mysqlx::SessionOption::DB, db_name_.c_str(),
                mysqlx::SessionOption::SSL_MODE, ssl_mode_);
            configureSessionTimeZone(*session);
            auto schema = std::make_unique<mysqlx::Schema>(session->getSchema(db_name_));
            thread_session_ = std::move(session);
            thread_schema_ = std::move(schema);
            std::cout << "✅ Database connection successful via SessionOption method! SSL mode: "
                      << sslModeToString(ssl_mode_) << std::endl;
            connected = true;
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "❌ SessionOption connection failed: " << e.what() << std::endl;
        }
    }

    if (!connected)
    {
        try
        {
            // 如果禁用了启动迁移，这里也不应偷偷执行 CREATE DATABASE 这类 DDL。
            if (!shouldRunStartupMigrations())
            {
                std::cerr << "Skipping automatic database creation because DB_AUTO_RUN_MIGRATIONS is disabled." << std::endl;
                throw std::runtime_error("Database does not exist and automatic migrations are disabled");
            }

            // 如果目标库不存在，先连接到服务端级别，再按需创建数据库。
            mysqlx::Session temp_session(
                mysqlx::SessionOption::HOST, db_host_.c_str(),
                mysqlx::SessionOption::PORT, db_port_,
                mysqlx::SessionOption::USER, db_user_.c_str(),
                mysqlx::SessionOption::PWD, db_pass_.c_str(),
                mysqlx::SessionOption::SSL_MODE, ssl_mode_);
            configureSessionTimeZone(temp_session);

            auto result = temp_session
                              .sql("SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = ?")
                              .bind(db_name_)
                              .execute();

            if (result.count() == 0)
            {
                std::cout << "Database '" << db_name_ << "' does not exist, creating it..." << std::endl;
                temp_session
                    .sql("CREATE DATABASE IF NOT EXISTS `" + db_name_ + "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
                    .execute();
                std::cout << "Database '" << db_name_ << "' created successfully!" << std::endl;
            }

            // 创建数据库连接
            auto session = std::make_unique<mysqlx::Session>(
                mysqlx::SessionOption::HOST, db_host_.c_str(),
                mysqlx::SessionOption::PORT, db_port_,
                mysqlx::SessionOption::USER, db_user_.c_str(),
                mysqlx::SessionOption::PWD, db_pass_.c_str(),
                mysqlx::SessionOption::DB, db_name_.c_str(),
                mysqlx::SessionOption::SSL_MODE, ssl_mode_);
            configureSessionTimeZone(*session);
            auto schema = std::make_unique<mysqlx::Schema>(session->getSchema(db_name_));
            thread_session_ = std::move(session);
            thread_schema_ = std::move(schema);
            connected = true;
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "❌ Server-first connection failed: " << e.what() << std::endl;
        }
    }

    if (!connected)
    {
        try
        {
            // 最后再尝试 host string 的 URI 形式，补充兜底。
            std::string host_and_port = db_host_ + ":" + std::to_string(db_port_);
            auto session = std::make_unique<mysqlx::Session>(
                mysqlx::SessionOption::URI,
                ("mysqlx://" + db_user_ + ":" + db_pass_ + "@" + host_and_port +
                 "/" + db_name_ + "?ssl-mode=" + std::string(sslModeToUriValue(ssl_mode_)))
                    .c_str());
            configureSessionTimeZone(*session);
            auto schema = std::make_unique<mysqlx::Schema>(session->getSchema(db_name_));
            thread_session_ = std::move(session);
            thread_schema_ = std::move(schema);
            std::cout << "✅ Database connection successful via host string method! SSL mode: "
                      << sslModeToString(ssl_mode_) << std::endl;
            connected = true;
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "❌ Host string connection failed: " << e.what() << std::endl;
        }
    }

    if (!connected)
    {
        std::cerr << "❌ All connection methods failed!" << std::endl;
        std::cerr << "连接参数详情:" << std::endl;
        std::cerr << "- Host: " << db_host_ << std::endl;
        std::cerr << "- Port: " << db_port_ << std::endl;
        std::cerr << "- User: " << db_user_ << std::endl;
        std::cerr << "- DB Name: " << db_name_ << std::endl;
        std::cerr << "- SSL Mode: " << sslModeToString(ssl_mode_) << std::endl;
        thread_session_.reset();
        thread_schema_.reset();
    }

    return connected;
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

void DatabaseManager::destroyInstance()
{
    // 显式释放单例，便于服务关闭时清理数据库资源。
    std::lock_guard<std::mutex> lock(mutex_);
    instance.reset();
}

mysqlx::Session *DatabaseManager::getSession()
{
    // 按需确保当前线程连接存在，再返回底层 session。
    return ensureThreadConnection() ? thread_session_.get() : nullptr;
}

mysqlx::Schema *DatabaseManager::getSchema()
{
    // schema 依赖 session，同样走线程级懒初始化。
    return ensureThreadConnection() ? thread_schema_.get() : nullptr;
}

DatabaseManager::~DatabaseManager()
{
    // 当前线程退出前释放持有的数据库连接对象。
    thread_session_.reset();
    thread_schema_.reset();
}
