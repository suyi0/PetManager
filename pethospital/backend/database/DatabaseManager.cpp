#include <iostream>
#include <mutex>
#include "DatabaseManager.h"

// 添加从配置文件读取数据库配置的函数
std::tuple<std::string, int, std::string, std::string, std::string> loadDatabaseConfig()
{
    // 默认数据库连接信息
    std::string host = "";
    int port = 0;
    std::string user = "";
    std::string password = "";
    std::string name = "";

    // 从环境变量获取数据库连接信息
    const char *db_host = getenv("DB_HOST");
    const char *db_port = getenv("DB_PORT");
    const char *db_user = getenv("DB_USER");
    const char *db_pass = getenv("DB_PASS");
    const char *db_name = getenv("DB_NAME");
    const char *db_config = getenv("PETMANAGERCONFIG_PATH") ? getenv("PETMANAGERCONFIG_PATH") : "config.json";
    if (db_host && db_port && db_user && db_pass && db_name)
    {
        host = std::string(db_host);
        port = std::stoi(std::string(db_port));
        user = std::string(db_user);
        password = std::string(db_pass);
        name = std::string(db_name);
    }

    // 如果环境变量没有设置，则尝试从配置文件加载
    if (host == "" || user == "" || password == "" || name == "")
    {
        std::cout << "⚠️  Warning: Some environment variables are not set, trying to load from config file..." << std::endl;

        std::ifstream configFile(db_config); // 读取文件
        if (configFile.is_open())
        {
            try
            {
                nlohmann::json config;
                configFile >> config; // 将整个文件内容解析为JSON对象

                if (config.contains("database"))
                {
                    if (config["database"].contains("host"))
                    {
                        host = config["database"]["host"].get<std::string>();
                    }
                    if (config["database"].contains("port"))
                    {
                        port = config["database"]["port"].get<int>();
                    }
                    if (config["database"].contains("user"))
                    {
                        user = config["database"]["user"].get<std::string>();
                    }
                    if (config["database"].contains("password"))
                    {
                        password = config["database"]["password"].get<std::string>();
                    }
                    if (config["database"].contains("schema"))
                    {
                        name = config["database"]["schema"].get<std::string>();
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "❌ Error parsing config.json: " << e.what() << std::endl;
                std::cerr << "❌ Failed to load database configuration!" << std::endl;
                throw std::runtime_error("Failed to load database configuration");
            }
        }
        else
        {
            std::cerr << "❌ Config file not found at: " << db_config << std::endl;
            std::cerr << "❌ Failed to load database configuration!" << std::endl;
            throw std::runtime_error("Failed to load database configuration");
        }
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
void DatabaseManager::create_Tables()
{
    static bool tables_created = false; // 添加标志防止重复执行

    if (tables_created)
    {
        std::cout << "create_Tables() called but tables already created, skipping." << std::endl;
        return; // 如果表已创建，则直接返回
    }

    std::cout << "create_Tables() executing for the first time..." << std::endl;

    // 检查并创建必要的表
    if (schema != nullptr)
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
                         "user_introduction, "                       // 用户简介(一般只有医生有介绍)
                         "user_level int, "                          // 用户等级(排列顺序：同等级按字母顺序排列，不同等级按等级顺序排列)
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "CONSTRAINT fk_user_type FOREIGN KEY (type_id) REFERENCES types(id)" // 外键约束

                         ")")
                .execute();
            std::cout << "users table created successfully." << std::endl;
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
                         "order_totalprice DECIMAL(100, 2), "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "                                   // 订单创建时间
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "       // 订单更新时间
                         "CONSTRAINT fk_orders_pet_id FOREIGN KEY (pet_id) REFERENCES pets(id), "
                         "CONSTRAINT fk_orders_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id), "
                         "INDEX idx_petId_time (pet_id, created_at) "
                         ")")
                .execute();
            
            std::cout << "orders table created successfully" << std::endl;
        }

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
                         "price DECIMAL(100, 2), "
                         "total_price DECIMAL(100, 2), "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "INDEX idx_orderId_time (order_id, created_at), "
                         "CONSTRAINT fk_order_id FOREIGN KEY (order_id) REFERENCES orders(id), "
                         "CONSTRAINT fk_medicine_id FOREIGN KEY (medicine_id) REFERENCES warehouse(id) "
                         ")")
                .execute();
        }

        // 创建仓库表
        if(warehouse_exists)
        {
            std::cout << "warehouse table is exists." << std::endl;
        }
        else
        {
            std::cout << "warehouse table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE warehouse("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "item_name VARCHAR(255), "                                                                               // 物品名称
                         "item_type VARCHAR(255), "                                                                               // 物品类型
                         "item_productiondate DATE, "                                                                             // 生产日期
                         "item_expirationdate DATE"                                                                               // 到期日期
                         "days_until_expire INT GENERATED ALWAYS AS (DATEDIFF(item_expirationdate, CURDATE())) STORED, "          // 剩余天数
                         "item_price DECIMAL(10, 2), "                                                                            // 价格
                         "item_number INT, "                                                                                      // 数量
                         "item_totalprice DECIMAL(100, 2), "                                                                      // 总价
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "                                                       // 创建时间
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "                           // 更新时间
                         "INDEX idx_id_exp (id, days_until_expire), "
                         "INDEX idx_exp (days_until_expire) "
                         ")")
                    .execute();
            std::cout << " warehouse table created successfully" << std::endl;
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
                        "CONSTRAINT fk_doctor_id FOREIGN KEY (userId) REFERENCES users(id), "
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
    // 初始化数据库 - 使用新版API
    try
    {
        // 加载数据库配置
        auto [db_host, db_port, db_user, db_pass, db_name] = loadDatabaseConfig();

        std::cout << "Attempting to connect to database..." << std::endl;
        std::cout << "Host: " << db_host << ", Port: " << db_port << ", User: " << db_user << ", DB: " << db_name << std::endl;

        // 尝试多种连接方法
        std::cout << "Trying various connection methods..." << std::endl;

        bool connected = false;
        
        // 方法1: 尝试使用 URI 连接
        try 
        {
            std::string full_uri = "mysqlx://" + db_user + ":" + db_pass + "@" + db_host + ":" + std::to_string(db_port) + "/" + db_name + "?ssl-mode=DISABLED&connect-timeout=30000";
            std::cout << "Trying URI connection: " << full_uri << std::endl;
            session = new mysqlx::Session(full_uri);                    // 建立会话
            schema = new mysqlx::Schema(session->getSchema(db_name));   // 获取数据库模式
            std::cout << "✅ Database connection successful via URI method!" << std::endl;
            connected = true;
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "❌ URI connection failed: " << e.what() << std::endl;
        }

        if (!connected) {
            // 方法2: 尝试使用 SessionOptions - 这是正确的方法
            try 
            {
                std::cout << "Trying SessionOption connection method..." << std::endl;
                session = new mysqlx::Session(
                    mysqlx::SessionOption::HOST, db_host.c_str(),
                    mysqlx::SessionOption::PORT, db_port,
                    mysqlx::SessionOption::USER, db_user.c_str(),
                    mysqlx::SessionOption::PWD, db_pass.c_str(),
                    mysqlx::SessionOption::DB, db_name.c_str(),
                    mysqlx::SessionOption::SSL_MODE, mysqlx::SSLMode::DISABLED
                );
                schema = new mysqlx::Schema(session->getSchema(db_name));
                std::cout << "✅ Database connection successful via SessionOption method!" << std::endl;
                connected = true;
            }
            catch (const mysqlx::Error &e)
            {
                std::cerr << "❌ SessionOption connection failed: " << e.what() << std::endl;
            }
        }

        if (!connected) {
            // 方法3: 尝试先连接到服务器，再选择数据库
            try 
            {
                std::cout << "Trying server-first connection method..." << std::endl;
                mysqlx::Session temp_session(
                    mysqlx::SessionOption::HOST, db_host.c_str(),
                    mysqlx::SessionOption::PORT, db_port,
                    mysqlx::SessionOption::USER, db_user.c_str(),
                    mysqlx::SessionOption::PWD, db_pass.c_str(),
                    mysqlx::SessionOption::SSL_MODE, mysqlx::SSLMode::DISABLED
                );
                
                // 检查并创建数据库
                auto result = temp_session.sql("SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = ?")
                                  .bind(db_name)
                                  .execute();

                if (result.count() == 0)
                {
                    std::cout << "Database '" << db_name << "' does not exist, creating it..." << std::endl;
                    temp_session.sql("CREATE DATABASE IF NOT EXISTS `" + db_name + "` CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci").execute();
                    std::cout << "Database '" << db_name << "' created successfully!" << std::endl;
                }
                
                // 现在连接到特定数据库
                session = new mysqlx::Session(
                    mysqlx::SessionOption::HOST, db_host.c_str(),
                    mysqlx::SessionOption::PORT, db_port,
                    mysqlx::SessionOption::USER, db_user.c_str(),
                    mysqlx::SessionOption::PWD, db_pass.c_str(),
                    mysqlx::SessionOption::DB, db_name.c_str(),
                    mysqlx::SessionOption::SSL_MODE, mysqlx::SSLMode::DISABLED
                );
                schema = new mysqlx::Schema(session->getSchema(db_name));
                std::cout << "✅ Database connection successful via server-first method!" << std::endl;
                connected = true;
            }
            catch (const mysqlx::Error &e)
            {
                std::cerr << "❌ Server-first connection failed: " << e.what() << std::endl;
            }
        }

        if (!connected) {
            // 方法4: 尝试使用字符串形式的连接参数
            try 
            {
                std::cout << "Trying host string connection method..." << std::endl;
                std::string host_and_port = db_host + std::string(":") + std::to_string(db_port);
                session = new mysqlx::Session(
                    mysqlx::SessionOption::URI, ("mysqlx://" + db_user + ":" + db_pass + "@" + host_and_port + "/" + db_name + "?ssl-mode=DISABLED").c_str()
                );
                schema = new mysqlx::Schema(session->getSchema(db_name));
                std::cout << "✅ Database connection successful via host string method!" << std::endl;
                connected = true;
            }
            catch (const mysqlx::Error &e)
            {
                std::cerr << "❌ Host string connection failed: " << e.what() << std::endl;
            }
        }

        if (!connected) {
            std::cerr << "❌ All connection methods failed!" << std::endl;
            std::cerr << "连接参数详情:" << std::endl;
            std::cerr << "- Host: " << db_host << std::endl;
            std::cerr << "- Port: " << db_port << std::endl;
            std::cerr << "- User: " << db_user << std::endl;
            std::cerr << "- DB Name: " << db_name << std::endl;

            // 即使数据库连接失败，服务器也应该继续运行
            session = nullptr;
            schema = nullptr;
        }
        
        // 如果成功建立了会话，创建表
        if (session != nullptr && schema != nullptr) {
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
        session = nullptr;
        schema = nullptr;
    }
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
    return session;
}

mysqlx::Schema *DatabaseManager::getSchema()
{
    return schema;
}

DatabaseManager::~DatabaseManager()
{
    if (session)
    {
        delete session; // 关闭数据库会话
    }
    if (schema)
    {
        delete schema; // 关闭数据库模式
    }
}