#pragma once
#include "mysqlx/xdevapi.h"
#include "DatabaseManagerInterface.h"
#include <thread>
#include <nlohmann/json.hpp>
#include <tuple>
#include <fstream>
#include <iostream>
#include <mutex>


class DatabaseManager : public DatabaseManagerInterface
{
private:
    static std::mutex mutex_; // 添加互斥锁确保线程安全

    static std::shared_ptr<DatabaseManagerInterface> instance;

    mysqlx::Session *session;
    mysqlx::Schema *schema;

    // 私有构造函数防止外部实例化
    DatabaseManager();

    bool hasValidConnection() const;

public:
    ~DatabaseManager();

    // 获取单例实例
    static std::shared_ptr<DatabaseManagerInterface> getInstance();

    static void destroyInstance();

    // 创建必要的表
    void create_Tables() override;

    mysqlx::Session *getSession() override;  // 重写基类的纯虚函数
    mysqlx::Schema *getSchema() override;  // 重写基类的纯虚函数

    // 删除拷贝构造函数和赋值运算符防止复制
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;
};
