#pragma once
#include "mysqlx/xdevapi.h"
#include "DatabaseManagerInterface.h"
#include <chrono>
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
    static thread_local std::unique_ptr<mysqlx::Session> thread_session_; // 线程本地会话
    static thread_local std::unique_ptr<mysqlx::Schema> thread_schema_; // 线程本地模式
    // 上次活性确认时刻：缓存的连接可能被 MySQL 重启 / wait_timeout 掐掉，
    // 复用前按节流间隔 ping 一次，死连接丢弃重建（修复"指针非空即认为可用"）
    static thread_local std::chrono::steady_clock::time_point thread_validated_at_;
    // 本线程会话自上次请求边界清理后是否交给过业务代码（endOfRequestCleanup 的零开销门控）
    static thread_local bool thread_session_used_;

    std::string db_host_;
    int db_port_ = 0;
    std::string db_user_;
    std::string db_pass_;
    std::string db_name_;
    mysqlx::SSLMode ssl_mode_ = mysqlx::SSLMode::REQUIRED; // SSL 模式

    // 私有构造函数防止外部实例化
    DatabaseManager();

    bool hasValidConnection() const;
    bool ensureThreadConnection();
    // 建立一条已选中目标库、UTC 时区的新连接；失败抛 mysqlx::Error
    std::unique_ptr<mysqlx::Session> openSessionWithDatabase() const;
    // 接管新连接为当前线程会话/模式
    void adoptThreadSession(std::unique_ptr<mysqlx::Session> session);

public:
    ~DatabaseManager();

    // 获取单例实例
    static std::shared_ptr<DatabaseManagerInterface> getInstance();

    static void destroyInstance();

    // 请求边界防御清理（由 DbSessionGuardMiddleware 调用）：
    // thread_local 会话被同线程所有请求复用，handler 带着未完成事务返回
    // 会污染下一个请求；这里统一 ROLLBACK 掐断污染链，回滚失败则丢弃连接。
    static void endOfRequestCleanup();

    // 创建必要的表
    void create_Tables() override;

    mysqlx::Session *getSession() override;  // 重写基类的纯虚函数
    mysqlx::Schema *getSchema() override;  // 重写基类的纯虚函数

    // 删除拷贝构造函数和赋值运算符防止复制
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;
};
