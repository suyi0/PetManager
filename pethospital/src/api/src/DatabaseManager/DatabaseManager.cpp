#include <iostream>
#include <mutex>
#include "DatabaseManager.h"

// 添加静态成员定义
std::mutex DatabaseManager::mutex_;
void DatabaseManager::create_Tables()
{
    static bool tables_created = false;  // 添加标志防止重复执行
    
    if (tables_created) {
        std::cout << "create_Tables() called but tables already created, skipping." << std::endl;
        return;  // 如果表已创建，则直接返回
    }

    std::cout << "create_Tables() executing for the first time..." << std::endl;

    // 检查并创建必要的表
    if (schema != nullptr)
    {
        // 获取所有表名
        auto tables = schema->getTables();
        bool Users_exists = false;
        bool onlineDoctors_exists = false;
        bool reservates_exists = false;
        bool Types_exists = false;
        for (const auto &table : tables)
        {
            std::string table_name = table.getName();
            if (table_name == "Users")
            {
                Users_exists = true;
            }
            else if (table_name == "onlineDoctors")
            {
                onlineDoctors_exists = true;
            }
            else if (table_name == "reaservations")
            {
                reservates_exists = true;
            }
            else if (table_name == "Types")
            {
                Types_exists = true;
            }
        }

        if (Types_exists)
        {
            std::cout << "Types table is exists." << std::endl;
        }
        else
        {
            std::cout << "Types table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE Types ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "  // 自增主键
                         "type VARCHAR(255)"
                         ")")
                .execute();

            // 插入初始数据
            session->sql("INSERT INTO Types (type) VALUES ('管理员'), ('普通用户'), ('医生')").execute();
            std::cout << "Types table created successfully." << std::endl;
        }

        if (Users_exists)
        {
            std::cout << "Users table is exists." << std::endl;
        }
        else
        {
            std::cout << "Users table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE Users ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "type_id INT, "                        // 用户类型（如：普通用户、医生、管理员等）
                         "name VARCHAR(255), "
                         "password VARCHAR(255), "
                         "phone VARCHAR(20), " 
                         "email VARCHAR(255), "
                         "birthday DATE, "
                         "creation_time DATETIME, "
                         "address_id VARCHAR(50), "
                         "head_image VARCHAR(255),"
                         "CONSTRAINT fk_user_type FOREIGN KEY (type_id) REFERENCES Types(id)"                      // 外键约束
                         ")")
                .execute();
            std::cout << "Users table created successfully." << std::endl;
        }

        if (onlineDoctors_exists)
        {
            std::cout << "onlineDoctors table is exists." << std::endl;
        }
        else
        {
            std::cout << "onlineDoctors table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE onlineDoctors ("
                         "doctor_id INT PRIMARY KEY, "              // 关联Users表的ID
                         "name VARCHAR(255), "                      // 医生姓名
                         "specialty VARCHAR(255), "                 // 医生专业
                         "CONSTRAINT fk_doctor_user FOREIGN KEY (doctor_id) REFERENCES Users(id)"               // 关联用户表
                         ")")
                .execute();
            std::cout << "onlineDoctors table created successfully." << std::endl;
        }

        if (reservates_exists)
        {
            std::cout << "reaservations table is exists." << std::endl;
        }
        else
        {
            std::cout << "reaservations table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE reaservations ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "user_id INT , "                                                        // 用户ID
                         "doctor_id INT, "                                                       // 医生ID
                         "date DATE, "                                                           // 预约日期
                         "time_slot VARCHAR(20), "                                               // 预约时间段
                         "status VARCHAR(20), "                                                  // 预约状态（如：已预约、已取消等）
                         "creation_time DATETIME, "                                              // 预约创建时间
                         "CONSTRAINT fk_user_id FOREIGN KEY (user_id) REFERENCES Users(id), "                       // 外键关联用户表
                         "CONSTRAINT fk_doctor_id FOREIGN KEY (doctor_id) REFERENCES onlineDoctors(doctor_id)"      // 外键关联医生表
                         ")")
                .execute();
            std::cout << "reaservations table created successfully." << std::endl;
        }
        tables_created = true;  // 标记表已创建
    }
    else
    {
        std::cerr << "Database schema is null, cannot check doctors table" << std::endl;
    }
}

DatabaseManager *DatabaseManager::instance = nullptr;

DatabaseManager::DatabaseManager()
{
    // 初始化数据库 - 使用新版API
    try
    {
        // 从环境变量获取数据库连接信息
        const char *db_host = std::getenv("DB_HOST") ? std::getenv("DB_HOST") : "localhost";
        const char *db_user = std::getenv("DB_USER") ? std::getenv("DB_USER") : "root";
        const char *db_pass = std::getenv("DB_PASS") ? std::getenv("DB_PASS") : "REMOVED_DB_PASS";
        const char *db_name = std::getenv("DB_NAME") ? std::getenv("DB_NAME") : "pethospital";

        // 首先尝试使用URI方式连接，使用SSL
        std::string uri = "mysqlx://" + std::string(db_user) + ":" + std::string(db_pass) + "@" + std::string(db_host) + ":33060/" + std::string(db_name);

        try
        {
            session = new mysqlx::Session(uri);
            schema = new mysqlx::Schema(session->getSchema(db_name));
            std::cout << "Database connection successful!" << std::endl;
        }
        catch (const mysqlx::Error &e)
        {
            std::cerr << "URI连接失败，尝试使用SessionOption方式: " << e.what() << std::endl;

            // 使用新的连接方式，不指定SSL模式（使用默认设置）
            session = new mysqlx::Session(mysqlx::SessionOption::HOST, db_host,
                                          mysqlx::SessionOption::PORT, 33060,
                                          mysqlx::SessionOption::USER, db_user,
                                          mysqlx::SessionOption::PWD, db_pass,
                                          mysqlx::SessionOption::DB, db_name);
            session->sql("SET NAMES 'utf8mb4'").execute(); // 设置字符集
            std::cout << "Database connection successful!" << std::endl;
        }
        // 设置字符集为 UTF-8
        try
        {
            session->sql("SET NAMES 'utf8mb4'").execute();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Warning: Failed to set character set: " << e.what() << std::endl;
        }

        create_Tables();
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "连接失败: " << e.what() << std::endl;
        // 即使数据库连接失败，服务器也应该继续运行
        session = nullptr;
        schema = nullptr;
    }
}

DatabaseManager *DatabaseManager::getInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance == nullptr)
    {
        instance = new DatabaseManager();
    }
    return instance;
}

void DatabaseManager::destroyInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance != nullptr)
    {
        delete instance;
        instance = nullptr;
    }
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