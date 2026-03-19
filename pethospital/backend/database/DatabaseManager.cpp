#include <algorithm>
#include <cctype>
#include <iostream>
#include <mutex>
#include "DatabaseManager.h"

namespace
{
// 去除引号
std::string trimQuotes(std::string value)
{
    if (value.size() >= 2)
    {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\''))
        {
            return value.substr(1, value.size() - 2);
        }
    }

    return value;
}

// 判断环境变量是否为真
bool isTruthyEnv(const char *value)
{
    if (!value)
    {
        return false;
    }

    std::string normalized = trimQuotes(std::string(value));
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::toupper);
    return normalized == "1" || normalized == "TRUE" || normalized == "YES" || normalized == "ON";
}

// 解析SSL模式
mysqlx::SSLMode parseSslMode(const char *ssl_mode_env)
{
    if (!ssl_mode_env)
    {
        // MySQL X authentication often requires a secure channel for PLAIN auth.
        return mysqlx::SSLMode::REQUIRED;
    }

    std::string ssl_mode = trimQuotes(ssl_mode_env);
    std::transform(ssl_mode.begin(), ssl_mode.end(), ssl_mode.begin(), ::toupper);

    if (ssl_mode == "DISABLED")
    {
        if (!isTruthyEnv(getenv("DB_ALLOW_INSECURE_SSL")))
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

// 加载数据库配置
std::tuple<std::string, int, std::string, std::string, std::string> loadDatabaseConfig()
{
    std::string host = "";
    int port = 0;
    std::string user = "";
    std::string password = "";
    std::string name = "";

    const char *db_host = getenv("DB_HOST");
    const char *db_port = getenv("DB_PORT");
    const char *db_user = getenv("DB_USER");
    const char *db_pass = getenv("DB_PASS");
    const char *db_name = getenv("DB_NAME");
    if (db_host && db_port && db_user && db_pass && db_name)
    {
        host = trimQuotes(std::string(db_host));
        port = std::stoi(std::string(db_port));
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

// 添加列
void add_Column_If_Not_Exists(const std::string &table_name, const std::string &column_name, const std::string &column_definition)
{
    try
    {
        // 检查列是否已存在
        auto result = DatabaseManager::getInstance()->getSession()->sql(
                                                                      "SELECT COUNT(*) as count FROM INFORMATION_SCHEMA.COLUMNS "
                                                                      "WHERE TABLE_SCHEMA = DATABASE() "
                                                                      "AND TABLE_NAME = ? "
                                                                      "AND COLUMN_NAME = ?")
                          .bind(table_name, column_name)
                          .execute();

        auto row = result.fetchOne();
        int count = row[0];

        if (count == 0)
        {
            // 列不存在，添加新列
            std::string sql = "ALTER TABLE " + table_name + " ADD COLUMN " + column_name + " " + column_definition;
            DatabaseManager::getInstance()->getSession()->sql(sql).execute();
            std::cout << "Added column '" << column_name << "' to table '" << table_name << "'" << std::endl;
        }
        else
        {
            std::cout << "Column '" << column_name << "' already exists in table '" << table_name << "'" << std::endl;
        }
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "Error adding column '" << column_name << "' to table '" << table_name << "': " << e.what() << std::endl;
    }
}

// 添加静态成员定义
std::mutex DatabaseManager::mutex_;
thread_local std::unique_ptr<mysqlx::Session> DatabaseManager::thread_session_ =
    nullptr;
thread_local std::unique_ptr<mysqlx::Schema> DatabaseManager::thread_schema_ =
    nullptr;

// 判断是否有有效的连接
bool DatabaseManager::hasValidConnection() const
{
    return thread_session_ != nullptr && thread_schema_ != nullptr;
}

void DatabaseManager::create_Tables()
{
    static bool tables_created = false; // 添加标志防止重复执行

    if (tables_created)
    {
        std::cout << "create_Tables() called but tables already created, skipping." << std::endl;
        return; // 如果表已创建，则直接返回
    }

    std::cout << "create_Tables() executing for the first time..." << std::endl;

    mysqlx::Session *session = getSession();
    mysqlx::Schema *schema = getSchema();

    // 检查并创建必要的表
    if (hasValidConnection())
    {
        // 获取所有表名
        auto tables = schema->getTables();
        bool users_exists = false;
        bool workTimes_exists = false;
        bool onlineDoctors_exists = false;
        bool reservates_exists = false;
        bool types_exists = false;
        bool orders_exists = false;
        bool orderMedicines_exists = false;
        bool pets_exists = false;
        bool warehouse_exists = false;
        bool workTimeRecords_exists = false;
        bool system_operations_exists = false;
        bool user_operations_exists = false;
        for (const auto &table : tables)
        {
            std::string table_name = table.getName();
            if (table_name == "users")
            {
                users_exists = true;
            }
            else if(table_name == "workTimes")
            {
                workTimes_exists = true;
            }
            else if (table_name == "onlineDoctors")
            {
                onlineDoctors_exists = true;
            }
            else if (table_name == "reaservations")
            {
                reservates_exists = true;
            }
            else if (table_name == "types")
            {
                types_exists = true;
            }
            else if (table_name == "orders")
            {
                orders_exists = true;
            }
            else if(table_name == "orderMedicines")
            {
                orderMedicines_exists = true;
            }
            else if(table_name == "pets")
            {
                pets_exists = true;
            }
            else if(table_name == "warehouse")
            {
                warehouse_exists = true;
            }
            else if(table_name == "workTimeRecords")
            {
                workTimeRecords_exists = true;
            }
            else if(table_name == "system_operations")
            {
                system_operations_exists = true;
            }
            else if(table_name == "user_operations")
            {
                user_operations_exists = true;
            }
        }

        //用户类型表
        if (types_exists)
        {
            std::cout << "Types table is exists." << std::endl;
        }
        else
        {
            std::cout << "types table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE types ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, " // 自增主键
                         "type VARCHAR(255)"
                         ""
                         ")")
                .execute();

            // 插入初始数据
            session->sql("INSERT INTO types (type) VALUES ('管理员'), ('医生'), ('普通用户')").execute();
            std::cout << "types table created successfully." << std::endl;
        }

        // 用户表
        if (users_exists)
        {
            std::cout << "users table is exists." << std::endl;
        }
        else
        {
            std::cout << "users table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE users ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "type_id INT, " // 用户类型（如：普通用户、医生、管理员等）
                         "name VARCHAR(255), "
                         "password VARCHAR(255), "
                         "phone VARCHAR(20), "
                         "email VARCHAR(255), "
                         "birthday DATE, "
                         "address_id int, "
                         "head_image VARCHAR(255),"
                         "user_specialty VARCHAR(255),"              // 如果是医生，则添加此字段信息
                         "user_introduction TEXT, "                  // 用户简介(一般只有医生有介绍)
                         "user_level int, "                          // 用户等级(排列顺序：同等级按字母顺序排列，不同等级按等级顺序排列)
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "CONSTRAINT fk_user_type FOREIGN KEY (type_id) REFERENCES types(id)" // 外键约束

                         ")")
                .execute();
            std::cout << "users table created successfully." << std::endl;
        }

        // 创建仓库表
        if(warehouse_exists)
        {
            std::cout << "warehouse table is exists." << std::endl;
            try
            {
                session->sql("ALTER TABLE warehouse "
                             "MODIFY COLUMN item_totalprice DECIMAL(18, 2) "
                             "GENERATED ALWAYS AS (item_price * item_number) STORED")
                    .execute();
                std::cout << "warehouse.item_totalprice migrated to generated column." << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cout << "warehouse.item_totalprice migration skipped: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cout << "warehouse table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE warehouse("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "item_name VARCHAR(255), "                                                                               // 物品名称
                         "item_type VARCHAR(255), "                                                                               // 物品类型
                         "item_productiondate DATE, "                                                                             // 生产日期
                         "item_expirationdate DATE, "                                                                             // 到期日期
                         "days_until_expire INT DEFAULT NULL, "                                                                   // 剩余天数
                         "item_price DECIMAL(10, 2), "                                                                            // 价格
                         "item_number INT, "                                                                                      // 数量
                         "item_totalprice DECIMAL(18, 2) GENERATED ALWAYS AS (item_price * item_number) STORED, "                 // 总价
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "                                                       // 创建时间
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "                           // 更新时间
                         "INDEX idx_id_exp (id, days_until_expire), "
                         "INDEX idx_exp (days_until_expire) "
                         ")")
                    .execute();
            std::cout << " warehouse table created successfully" << std::endl;
        }

        // 上班时间表
        if(workTimes_exists)
        {
            std::cout << "workTimes table is exists." << std::endl;
        }
        else
        {
            std::cout << "workTimes table is not exists." << std::endl;
            session->sql("CREATE TABLE workTimes ("
                         "check_in_time_start TIME, "
                         "check_in_time_end TIME, "
                         "check_out_time_start TIME,"
                         "check_out_time_end TIME "
                         ")")
                         .execute();
            std::cout << "workTimes table created successfully." << std::endl;
        }

        // 添加医生上班表
        if (onlineDoctors_exists)
        {
            std::cout << "onlineDoctors table is exists." << std::endl;
        }
        else
        {
            std::cout << "onlineDoctors table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE onlineDoctors ( "
                         "doctor_id INT PRIMARY KEY, "                                              // 关联users表的ID
                         "date DATE, "                                                              // 上班日期
                         "check_in_time TIME, "                                                     // 上班时间
                         "check_out_time TIME, "                                                    // 下班时间
                         "status ENUM('online', 'offline'), "                                        // 在线状态
                         "CONSTRAINT fk_doctor_user FOREIGN KEY (doctor_id) REFERENCES users(id) "  // 关联用户表
                         ")")
                .execute();
            std::cout << "onlineDoctors table created successfully." << std::endl;
        }

        // 添加预约表
        if (reservates_exists)
        {
            std::cout << "reaservations table is exists." << std::endl;
        }
        else
        {
            std::cout << "reaservations table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE reaservations ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "                                                 // 预约ID，主键           
                         "user_id INT , "                                                                      // 用户ID
                         "doctor_id INT, "                                                                     // 医生ID
                         "date DATE, "                                                                         // 预约日期
                         "time_slot VARCHAR(20), "                                                             // 预约时间段
                         "status VARCHAR(20), "                                                                // 预约状态（如：已预约、已取消等）
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "                                    // 预约创建时间
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "        // 预约更新时间
                         "CONSTRAINT fk_user_id FOREIGN KEY (user_id) REFERENCES users(id), "                  // 外键关联用户表
                         "CONSTRAINT fk_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id), "              // 外键关联用户表的医生ID
                         "INDEX idx_userId_creationTime (user_id, created_at) "
                         ")")
                .execute();
            std::cout << "reaservations table created successfully." << std::endl;
        }

        // 创建宠物表
        if(pets_exists)
        {
            std::cout << "pets table is exists." << std::endl;
        }
        else
        {
            std::cout << "pets table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE pets ( "
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "user_id INT, "
                         "pet_name VARCHAR(255), "
                         "pet_type VARCHAR(255), "
                         "pet_age VARCHAR(255), "
                         "pet_sex VARCHAR(255) "
                        ")")
                .execute();
            std::cout << "pets table created successfully" << std::endl;
        }

        // 创建订单表
        if(orders_exists)
        {
            std::cout << "orders table is exists." << std::endl;
        }
        else
        {
            std::cout << "orders table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE orders ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "pet_id INT, "
                         "doctor_id INT, "
                         "order_type VARCHAR(255),"
                         "order_date VARCHAR(255), "
                         "order_status VARCHAR(255), "
                         "order_totalprice DECIMAL(18, 2), "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "                                   // 订单创建时间
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "       // 订单更新时间
                         "CONSTRAINT fk_orders_pet_id FOREIGN KEY (pet_id) REFERENCES pets(id), "
                         "CONSTRAINT fk_orders_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id), "
                         "INDEX idx_petId_time (pet_id, created_at) "
                         ")")
                .execute();
            
            std::cout << "orders table created successfully" << std::endl;
        }

        // 创建订单药品表
        if(orderMedicines_exists)
        {
            std::cout << "orderMedicines table is exists." << std::endl;
        }
        else
        {
            std::cout << "orderMedicines table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE orderMedicines ( "
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "order_id INT, "
                         "medicine_id INT, "
                         "quantity INT, "
                         "price DECIMAL(18, 2), "
                         "total_price DECIMAL(18, 2), "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "INDEX idx_orderId_time (order_id, created_at), "
                         "CONSTRAINT fk_order_id FOREIGN KEY (order_id) REFERENCES orders(id), "
                         "CONSTRAINT fk_medicine_id FOREIGN KEY (medicine_id) REFERENCES warehouse(id) "
                         ")")
                .execute();

            std::cout << "orderMedicines table created successfully" << std::endl;
        }

        // 创建上班时间记录表
        if(workTimeRecords_exists)
        {
            std::cout << "workTimeRecords table already exists." << std::endl;
        }
        else
        {
            std::cout << "workTimeRecords table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE workTimeRecords( "
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "doctor_id INT NOT NULL, "
                        "date DATE, "
                        "check_in_time TIME, "
                        "check_out_time TIME, "
                        "status ENUM('pending', 'approved', 'rejected'), "                          // 记录工作状态 （待定， 批准， 拒绝）
                        "notes TEXT, "                                                              // 备注
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                        "CONSTRAINT fk_worktime_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id), "
                        "INDEX idx_user (doctor_id)"
                        ")")
                    .execute();
            std::cout << "workTimeRecords table created successfully." << std::endl;
        }

        // 创建系统操作记录表
        if(system_operations_exists)
        {
            std::cout << "system_operations table is exists." << std::endl;
        }
        else
        {
            std::cout << "system_operations table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE system_operations( "
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "operation VARCHAR(100) NOT NULL, "
                        "details TEXT, "
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                        "INDEX idx_time (created_at)"
                        ")")
                    .execute();
            std::cout << "system_operations table created successfully." << std::endl;
        }

        // 创建用户操作记录表
        if(user_operations_exists)
        {
            std::cout << "user_operations table is exists." << std::endl;
        }
        else
        {
            std::cout << "user_operations table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE user_operations( "
                         "id INT AUTO_INCREMENT PRIMARY KEY, "
                         "user_id INT NOT NULL, "
                         "operation VARCHAR(100) NOT NULL, "
                         "details TEXT, "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "INDEX idx_user_time (user_id, created_at)"
                          ")").execute();
            std::cout << "user_operations table created successfully." << std::endl;
        }


        tables_created = true; // 标记表已创建
    }
    else
    {
        std::cerr << "Database schema is null, cannot check doctors table" << std::endl;
    }
}

std::shared_ptr<DatabaseManagerInterface> DatabaseManager::instance = nullptr;

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
        ssl_mode_ = parseSslMode(getenv("DB_SSL_MODE"));

        if (ensureThreadConnection()) // 线程连接上进行创建表操作
        {
            create_Tables();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ 数据库连接发生异常: " << e.what() << std::endl;
        std::cerr << "连接参数详情:" << std::endl;
        std::cerr << "- Host: " << getenv("DB_HOST") << std::endl;
        std::cerr << "- Port: " << getenv("DB_PORT") << std::endl;
        std::cerr << "- User: " << getenv("DB_USER") << std::endl;
        std::cerr << "- Password: " << (getenv("DB_PASS") ? "***" : "未设置") << std::endl;
        std::cerr << "- DB Name: " << getenv("DB_NAME") << std::endl;

        // 即使数据库连接失败，服务器也应该继续运行
        thread_session_.reset();
        thread_schema_.reset();
    }
}

// 确保线程连接
bool DatabaseManager::ensureThreadConnection()
{
    if (thread_session_ && thread_schema_)
    {
        return true;
    }

    bool connected = false;

    try
    {
        std::string full_uri =
            "mysqlx://" + db_user_ + ":" + db_pass_ + "@" + db_host_ + ":" +
            std::to_string(db_port_) + "/" + db_name_ +
            "?connect-timeout=30000&ssl-mode=" + sslModeToUriValue(ssl_mode_);
        auto session = std::make_unique<mysqlx::Session>(full_uri);
        auto schema =
            std::make_unique<mysqlx::Schema>(session->getSchema(db_name_));
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
            auto session = std::make_unique<mysqlx::Session>(
                mysqlx::SessionOption::HOST, db_host_.c_str(),
                mysqlx::SessionOption::PORT, db_port_,
                mysqlx::SessionOption::USER, db_user_.c_str(),
                mysqlx::SessionOption::PWD, db_pass_.c_str(),
                mysqlx::SessionOption::DB, db_name_.c_str(),
                mysqlx::SessionOption::SSL_MODE, ssl_mode_);
            auto schema =
                std::make_unique<mysqlx::Schema>(session->getSchema(db_name_));
            thread_session_ = std::move(session);
            thread_schema_ = std::move(schema);
            std::cout
                << "✅ Database connection successful via SessionOption method! SSL mode: "
                << sslModeToString(ssl_mode_) << std::endl;
            connected = true;
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "❌ SessionOption connection failed: " << e.what()
                      << std::endl;
        }
    }

    if (!connected)
    {
        try
        {
            mysqlx::Session temp_session(
                mysqlx::SessionOption::HOST, db_host_.c_str(),
                mysqlx::SessionOption::PORT, db_port_,
                mysqlx::SessionOption::USER, db_user_.c_str(),
                mysqlx::SessionOption::PWD, db_pass_.c_str(),
                mysqlx::SessionOption::SSL_MODE, ssl_mode_);

            auto result = temp_session
                              .sql("SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = ?")
                              .bind(db_name_)
                              .execute();

            if (result.count() == 0)
            {
                std::cout << "Database '" << db_name_
                          << "' does not exist, creating it..." << std::endl;
                temp_session
                    .sql("CREATE DATABASE IF NOT EXISTS `" + db_name_ +
                         "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
                    .execute();
                std::cout << "Database '" << db_name_
                          << "' created successfully!" << std::endl;
            }

            auto session = std::make_unique<mysqlx::Session>(
                mysqlx::SessionOption::HOST, db_host_.c_str(),
                mysqlx::SessionOption::PORT, db_port_,
                mysqlx::SessionOption::USER, db_user_.c_str(),
                mysqlx::SessionOption::PWD, db_pass_.c_str(),
                mysqlx::SessionOption::DB, db_name_.c_str(),
                mysqlx::SessionOption::SSL_MODE, ssl_mode_);
            auto schema =
                std::make_unique<mysqlx::Schema>(session->getSchema(db_name_));
            thread_session_ = std::move(session);
            thread_schema_ = std::move(schema);
            connected = true;
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "❌ Server-first connection failed: " << e.what()
                      << std::endl;
        }
    }

    if (!connected)
    {
        try
        {
            std::string host_and_port =
                db_host_ + std::string(":") + std::to_string(db_port_);
            auto session = std::make_unique<mysqlx::Session>(
                mysqlx::SessionOption::URI,
                ("mysqlx://" + db_user_ + ":" + db_pass_ + "@" + host_and_port +
                 "/" + db_name_ + "?ssl-mode=" +
                 std::string(sslModeToUriValue(ssl_mode_)))
                    .c_str());
            auto schema =
                std::make_unique<mysqlx::Schema>(session->getSchema(db_name_));
            thread_session_ = std::move(session);
            thread_schema_ = std::move(schema);
            std::cout
                << "✅ Database connection successful via host string method! SSL mode: "
                << sslModeToString(ssl_mode_) << std::endl;
            connected = true;
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "❌ Host string connection failed: " << e.what()
                      << std::endl;
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
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance == nullptr)
    {
        instance = std::shared_ptr<DatabaseManagerInterface>(new DatabaseManager());
    }
    return instance;
}

void DatabaseManager::destroyInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    instance.reset();
}

mysqlx::Session *DatabaseManager::getSession()
{
    return ensureThreadConnection() ? thread_session_.get() : nullptr;
}

mysqlx::Schema *DatabaseManager::getSchema()
{
    return ensureThreadConnection() ? thread_schema_.get() : nullptr;
}

DatabaseManager::~DatabaseManager()
{
    thread_session_.reset();
    thread_schema_.reset();
}
