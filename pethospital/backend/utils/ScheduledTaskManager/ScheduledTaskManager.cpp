#include "ScheduledTaskManager.h"
#include <iostream>
#include <sstream>
#ifdef __linux__
#include <sys/sysinfo.h>
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#include <psapi.h>
#elif __APPLE__
#include <mach/mach.h>
#include <sys/sysctl.h>
#endif

std::unique_ptr<ScheduledTaskManager> ScheduledTaskManager::instance = nullptr;
std::mutex ScheduledTaskManager::instanceMutex;

namespace
{
bool canUseDatabase(const std::shared_ptr<DatabaseManagerInterface> &dbManager)
{
    return dbManager && dbManager->getSession() && dbManager->getSchema();
}
}

ScheduledTaskManager::~ScheduledTaskManager()
{
    stop();
}

ScheduledTaskManager *ScheduledTaskManager::getInstance()
{
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance)
    {
        instance.reset(new ScheduledTaskManager());
    }
    return instance.get();
}

void ScheduledTaskManager::initialize(std::shared_ptr<DatabaseManagerInterface> dbMgr)
{
    this->dbManager = dbMgr;
    this->logger = std::make_shared<OperationLogger>(dbMgr);
    this->updater = std::make_shared<update>(dbMgr);


    // 获取当天 00:00 的时间点
    auto now = std::chrono::system_clock::now();                        // 获取当前时间点的time_point对象
    auto time_t_now = std::chrono::system_clock::to_time_t(now);        // 将当前时间点转换为 time_t 格式
    // time_t 是一个标量类型（通常是 long 或 long long），表示自 Unix 纪元（1970-01-01 00:00:00 UTC）以来的秒数

    // 转换为 tm 结构体后，可以直观地修改各个字段：
    std::tm local_tm = safeLocalTime(time_t_now);                       // 转换为本地时间结构体 tm
    local_tm.tm_hour = 0;                                               // 设置时为0
    local_tm.tm_min = 0;                                                // 设置分为0        
    local_tm.tm_sec = 0;                                                // 设置秒为0     
    // std::mktime(local_tm) 将本地时间结构体 tm* 转换为 time_t 类型（自 Unix 纪元以来的秒数）。
    auto midnight = std::chrono::system_clock::from_time_t(std::mktime(&local_tm));  // 将午夜时间转换回 chrono::system_clock 类型

    // 添加默认的30分钟系统信息记录任务
    addTask("SystemInfoRecord", [this]()
            { this->recordSystemInfo(); }, std::chrono::minutes(30), midnight);
    addTask("DatabaseStatus", [this]()
            { this->recordDatabaseStatus(); }, std::chrono::minutes(30), midnight);
    addTask("MemoryUsage", [this]()
            { this->recordMemoryUsage(); }, std::chrono::minutes(30), midnight);
    addTask("UserActivity", [this]()
            { this->recordUserActivity(); }, std::chrono::minutes(30), midnight);
    
    // 添加30分钟，自动更新记录任务
    addTask("Automatic_update", [this]()
            { this->Automatic_update(); }, std::chrono::minutes(30), midnight);
}

void ScheduledTaskManager::start()
{
    if (running.exchange(true))
    {
        return; // 已经在运行
    }

    workerThread = std::thread([this]()
                               { this->workerLoop(); }); // 创建并启动线程
    std::cout << "定时任务管理器已启动" << std::endl;
}

void ScheduledTaskManager::stop()
{
    if (!running.exchange(false))
    {           // 修改running状态为false，标志线程停止
        return; // 已经停止
    }

    if (workerThread.joinable())
    {
        workerThread.join(); // 等待线程结束(防止线程意外退出)
    }

    std::cout << "定时任务管理器已停止" << std::endl;
}

void ScheduledTaskManager::addTask(const std::string &name, std::function<void()> taskFunction, std::chrono::minutes interval, std::chrono::system_clock::time_point startTime)
{
    std::lock_guard<std::mutex> lock(tasksMutex);

    // 检查是否已存在同名任务,找到返回当前任务迭代器，否者返回.end()迭代器
    // std::find_if(开头迭代器, 结束迭代器, 筛选条件函数)
    auto it = std::find_if(tasks.begin(), tasks.end(),
                           [&name](const Task &t)
                           { return t.taskName == name; }); // lambda表达式
    // 捕获列表 [&name]
    // 以引用方式捕获外部变量 name，使 Lambda 内部可以访问它。

    // 参数列表(const Task &t)
    // 接收一个 Task 类型的常量引用参数 t，表示当前遍历到的任务对象。

    // 函数体 { return t.taskName == name; }
    // 比较任务对象 t 的成员 taskName 是否等于外部变量 name，并返回布尔值。

    if (it != tasks.end())
    {
        // 更新现有任务
        it->taskFunction = taskFunction;
        it->interval = interval;
        it->lastExecution = startTime;
        it->lastExecutedMinute = -1; // 重置分钟标记
    }
    else
    {
        // 添加新任务
        tasks.push_back({name,
                         taskFunction,
                         interval,
                         startTime,
                         -1}); // -1 表示尚未执行
    }

    std::cout << "已添加定时任务: " << name << " (间隔: " << interval.count() << " 分钟)" << std::endl;
}

void ScheduledTaskManager::removeTask(const std::string &name)
{
    std::lock_guard<std::mutex> lock(tasksMutex);

    auto it = std::find_if(tasks.begin(), tasks.end(),
                           [&name](const Task &t)
                           { return t.taskName == name; });

    if (it != tasks.end())
    {
        tasks.erase(it);
        std::cout << "已移除定时任务: " << name << std::endl;
    }
}

// 测试用
void ScheduledTaskManager::executeAllTasks()
{
    std::lock_guard<std::mutex> lock(tasksMutex);

    for (auto &task : tasks)
    {
        try
        {
            task.taskFunction();
            task.lastExecution = std::chrono::system_clock::now();
            std::cout << "执行定时任务: " << task.taskName << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "执行定时任务 " << task.taskName << " 失败: " << e.what() << std::endl;
        }
    }
}

void ScheduledTaskManager::workerLoop()
{
    while (running)
    {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm = safeLocalTime(time_t_now);

        // 计算当前时间的分钟数（从 00:00 开始的总分钟数）
        int currentTotalMinutes = local_tm.tm_hour * 60 + local_tm.tm_min;
        // 计算应该执行的时间节点（0 或 30）
        int scheduledMinute = (currentTotalMinutes / 30) * 30;

        {
            std::lock_guard<std::mutex> lock(tasksMutex);

            for (auto &task : tasks)
            {
                // 检查是否到了新的 30 分钟节点，且该节点尚未执行
                if (scheduledMinute > task.lastExecutedMinute && 
                    scheduledMinute % 30 == 0)
                {
                    try
                    {
                        task.taskFunction();
                        task.lastExecution = now;
                        task.lastExecutedMinute = scheduledMinute;

                        std::cout << "[定时任务] 执行完成：" << task.taskName 
                                  << " (时间节点：" << (scheduledMinute / 60) << ":" 
                                  << (scheduledMinute % 60 == 0 ? "00" : "30") << ")" << std::endl;
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "执行定时任务 " << task.taskName << " 失败: " << e.what() << std::endl;
                    }
                }
            }
        }

        // 休眠1分钟检查一次
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }
}

// addTask() 函数中添加的执行任务
void ScheduledTaskManager::recordSystemInfo()
{
    std::stringstream ss;
    ss << "系统信息记录 - ";
    ss << "时间: " << getCreateTime();

#ifdef __linux__
    ss << ", 平台: Linux";
#elif _WIN32
    ss << ", 平台: Windows";
#elif __APPLE__
    ss << ", 平台: macOS";
#endif

    // 记录到数据库
    if (logger && canUseDatabase(dbManager))
    {
        logger->logSystemOperation(dbManager, "SystemInfo", ss.str());
    }

    std::cout << "系统信息已记录: " << ss.str() << std::endl;
}

void ScheduledTaskManager::recordDatabaseStatus()
{
    if (!canUseDatabase(dbManager))
        return;

    try
    {
        // information_schema.tables：MySQL 的系统信息表，存储所有数据库的元数据。
        // table_schema = DATABASE()：筛选条件，DATABASE() 返回当前连接的数据库名，即只统计当前数据库的表。
        auto result = dbManager->getSession()->sql("SELECT COUNT(*) as table_count FROM information_schema.tables WHERE table_schema = DATABASE()").execute().fetchOne();

        std::stringstream ss;
        ss << "数据库状态 - 表数量: " << result[0].get<int>();

        // 记录到数据库
        if (logger)
        {
            logger->logSystemOperation(dbManager, "DatabaseStatus", ss.str());
        }

        std::cout << "数据库状态已记录: " << ss.str() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "记录数据库状态失败: " << e.what() << std::endl;
    }
}

void ScheduledTaskManager::recordMemoryUsage()
{
    std::stringstream ss;
    ss << "内存使用情况 - ";

#ifdef __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0)
    {
        long total_ram = info.totalram * info.mem_unit;
        long free_ram = info.freeram * info.mem_unit;
        ss << "总内存: " << total_ram / (1024 * 1024) << "MB, "
           << "可用内存: " << free_ram / (1024 * 1024) << "MB";
    }
#elif _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo))
    {
        ss << "总内存: " << memInfo.ullTotalPhys / (1024 * 1024) << "MB, "
           << "可用内存: " << memInfo.ullAvailPhys / (1024 * 1024) << "MB";
    }
#elif __APPLE__
    vm_size_t page_size;
    mach_port_t mach_port;
    mach_msg_type_number_t count;
    vm_statistics64_data_t vm_stats;

    mach_port = mach_host_self();
    count = HOST_VM_INFO64_COUNT;

    if (KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
        KERN_SUCCESS == host_statistics64(mach_port, HOST_VM_INFO64,
                                          (host_info64_t)&vm_stats, &count))
    {
        uint64_t total_memory = (vm_stats.active_count + vm_stats.inactive_count +
                                 vm_stats.wire_count + vm_stats.free_count) *
                                page_size;
        uint64_t free_memory = vm_stats.free_count * page_size;

        ss << "总内存: " << total_memory / (1024 * 1024) << "MB, "
           << "可用内存: " << free_memory / (1024 * 1024) << "MB";
    }
#endif

    // 记录到数据库
    if (logger && canUseDatabase(dbManager))
    {
        logger->logSystemOperation(dbManager, "MemoryUsage", ss.str());
    }

    std::cout << "内存使用情况已记录: " << ss.str() << std::endl;
}

void ScheduledTaskManager::recordUserActivity()
{
    if (!canUseDatabase(dbManager))
        return;

    try
    {
        // 查询最近30分钟的用户操作记录
        auto result = dbManager->getSession()->sql("SELECT COUNT(*) as recent_operations FROM user_operations WHERE created_at > DATE_SUB(NOW(), INTERVAL 30 MINUTE)").execute().fetchOne();

        std::stringstream ss;
        ss << "用户活动统计 - 最近30分钟操作数: " << result[0].get<int>();

        // 记录到数据库
        if (logger)
        {
            logger->logSystemOperation(dbManager, "UserActivity", ss.str());
        }

        std::cout << "用户活动统计已记录: " << ss.str() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "记录用户活动统计失败: " << e.what() << std::endl;
    }
}

void ScheduledTaskManager::Automatic_update()
{ 
    if (!dbManager)
        return;
    try
    {
        if(updater)
        {
            updater->Automatic_update();
        }
        std::cout << "系统自动更新已完成" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "系统自动更新失败: " << e.what() << std::endl;
    }
}
