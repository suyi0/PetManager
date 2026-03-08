#ifndef SCHEDULEDTASKMANAGER_H
#define SCHEDULEDTASKMANAGER_H

#include "../Utils.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>
#include <memory>
#include "../../controllers/OperationLogger/OperationLogger.h"
#include "../../controllers/update/update.h"

class ScheduledTaskManager
{
private:
    static std::unique_ptr<ScheduledTaskManager> instance;      // 单例实例
    static std::mutex instanceMutex;                            // 互斥锁

    std::atomic<bool> running;                                  // 工作线程运行标志
    std::thread workerThread;                                   // 工作线程
    std::shared_ptr<DatabaseManagerInterface> dbManager;                        // 数据库管理器
    std::shared_ptr<OperationLogger> logger;                                    // 操作日志记录器
    std::shared_ptr<update> updater;                                            // 更新管理器       

    // 存储定时任务
    struct Task
    {
        std::string taskName;                                   // 任务名称
        std::function<void()> taskFunction;                     // 任务函数
        std::chrono::minutes interval;                          // 任务执行间隔
        std::chrono::system_clock::time_point lastExecution;    // 上次执行时间
        int lastExecutedMinute;                                 // 上次执行的分钟数（用于判断是否到了下一个 30 分钟节点）
    };

    std::vector<Task> tasks;
    std::mutex tasksMutex;

    // 私有构造函数
    ScheduledTaskManager() : running(false), dbManager(nullptr), logger(nullptr), updater(nullptr) {};

    // 工作线程主循环
    void workerLoop();

    // 执行系统信息记录任务
    void recordSystemInfo();

    // 记录数据库状态
    void recordDatabaseStatus();

    // 记录内存使用情况
    void recordMemoryUsage();

    // 记录用户活动统计
    void recordUserActivity();

    // 执行30分钟检测是否需要存储员工考勤信息
    void Automatic_update();

public:
    ~ScheduledTaskManager();

    // 获取单例实例
    static ScheduledTaskManager *getInstance();

    // 初始化定时任务管理器
    void initialize(std::shared_ptr<DatabaseManagerInterface> dbMgr);

    // 启动定时任务
    void start();

    // 停止定时任务
    void stop();

    // 添加自定义定时任务
    void addTask(const std::string &name, std::function<void()> task, std::chrono::minutes interval, std::chrono::system_clock::time_point startTime);

    // 移除定时任务
    void removeTask(const std::string &name);

    // 立即执行所有任务（用于测试）
    void executeAllTasks();

    // 禁止拷贝
    ScheduledTaskManager(const ScheduledTaskManager &) = delete;
    
    ScheduledTaskManager &operator=(const ScheduledTaskManager &) = delete;
};

#endif