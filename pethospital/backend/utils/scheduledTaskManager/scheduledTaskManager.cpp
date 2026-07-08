#include "scheduledTaskManager.h"
#include "../../services/redis/RedisClient.h"
#include "../../services/redis/redisLock/RedisLock.h"
#include <iostream>
#include <sstream>
#include <cstdio>
#include <unistd.h>
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

    // 将日期和时间转换为字符串格式 "YYYY-MM-DD HH:MM:SS"
    std::string toDateTimeString(
        const boost::gregorian::date &date,
        const boost::posix_time::time_duration &time)
    {
        return formatDateTime(boost::posix_time::ptime(date, time));
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
    this->updater.emplace(dbMgr);

    // 获取当天 00:00 的时间点
    auto now = std::chrono::system_clock::now();                 // 获取当前时间点的time_point对象
    auto time_t_now = std::chrono::system_clock::to_time_t(now); // 将当前时间点转换为 time_t 格式
    // time_t 是一个标量类型（通常是 long 或 long long），表示自 Unix 纪元（1970-01-01 00:00:00 UTC）以来的秒数

    // 转换为 tm 结构体后，可以直观地修改各个字段：
    std::tm local_tm = safeLocalTime(time_t_now); // 转换为本地时间结构体 tm
    local_tm.tm_hour = 0;                         // 设置时为0
    local_tm.tm_min = 0;                          // 设置分为0
    local_tm.tm_sec = 0;                          // 设置秒为0
    // std::mktime(local_tm) 将本地时间结构体 tm* 转换为 time_t 类型（自 Unix 纪元以来的秒数）。
    auto midnight = std::chrono::system_clock::from_time_t(std::mktime(&local_tm)); // 将午夜时间转换回 chrono::system_clock 类型

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

    // 添加每日24时执行更新员工工资记录任务
    addTask("Automatic_update_salaryRecord", [this]()
            { this->Automatic_update_salaryRecord(); }, std::chrono::hours(24), midnight);
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

    // notify_all() 唤醒线程stopCv.
    stopCv.notify_all();

    // 如果线程已被启动并且还没有被 join() 或 detach()，则 joinable() 返回 true
    // 如果线程没有被启动、已经被 join() 或已经被 detach()，则返回 false
    if (workerThread.joinable())
    {
        workerThread.join(); // 等待线程结束(防止主线程继续往下走时，后台线程还没收尾完)
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
                    // 多实例分布式锁：同一任务、同一时间节点只允许一个实例执行。
                    // 锁键含日期+时段，天然每个时段唯一；抢不到说明别的实例已执行该时段。
                    // Redis 不可用时 acquireSlot=true，退化为单实例直接执行（不阻断业务）。
                    bool acquireSlot = true;
                    if (RedisClient::instance().enabled())
                    {
                        char slotKey[160];
                        std::snprintf(slotKey, sizeof(slotKey),
                                      "schedlock:%s:%04d%02d%02d:%d",
                                      task.taskName.c_str(),
                                      local_tm.tm_year + 1900, local_tm.tm_mon + 1,
                                      local_tm.tm_mday, scheduledMinute);
                        // TTL 1 小时：键按时段唯一，TTL 仅用于自动回收，不影响下一时段。
                        // 占位（不释放，靠 TTL 回收）；三态降级：抢到/出错都执行（出错=退化单实例不漏跑），明确被占=跳过。
                        RedisLock::Outcome claim = RedisLock::tryClaim(
                            slotKey, 3600, std::to_string(::getpid()));
                        acquireSlot = (claim != RedisLock::Outcome::Contended);
                    }

                    if (acquireSlot)
                    {
                        try
                        {
                            task.taskFunction();

                            std::cout << "[定时任务] 执行完成：" << task.taskName
                                      << " (时间节点：" << (scheduledMinute / 60) << ":"
                                      << (scheduledMinute % 60 == 0 ? "00" : "30") << ")" << std::endl;
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << "执行定时任务 " << task.taskName << " 失败: " << e.what() << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "[定时任务] 跳过（其他实例已执行该时段）：" << task.taskName
                                  << " (时间节点：" << (scheduledMinute / 60) << ":"
                                  << (scheduledMinute % 60 == 0 ? "00" : "30") << ")" << std::endl;
                    }

                    // 无论本实例是否执行，都推进本地标记，避免下个循环重复尝试。
                    task.lastExecution = now;
                    task.lastExecutedMinute = scheduledMinute;
                }
            }
        }

        // 可中断等待，避免 stop() 后还要额外卡住 1 分钟
        std::unique_lock<std::mutex> lock(stopMutex);

        // wait_for(锁对象，等待时长，可选的谓词条件（一般是判断wait_for什么情况提前结束等待）)
        // 返回 true 表示谓语成立了，返回 false 表示超时或者谓语不成立
        // 如果 lambda 返回 true，wait_for 立即返回 true
        // 如果一直到超时条件都没成立，就返回 false
        stopCv.wait_for(lock, std::chrono::minutes(1), [this]()
                        { return !running.load(); }); // running.load() 函数返回 running 状态
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
    if (canUseDatabase(dbManager))
    {
        OperationLogger::logSystemOperation(dbManager, "系统信息", ss.str());
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
        if (canUseDatabase(dbManager))
        {
            OperationLogger::logSystemOperation(dbManager, "数据库状态", ss.str());
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
    if (canUseDatabase(dbManager))
    {
        OperationLogger::logSystemOperation(dbManager, "内存使用量", ss.str());
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
        if (canUseDatabase(dbManager))
        {
            OperationLogger::logSystemOperation(dbManager, "用户行为", ss.str());
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
        if (updater.has_value())
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

void ScheduledTaskManager::Automatic_update_salaryRecord()
{
    if (!canUseDatabase(dbManager))
        return;

    try
    {
        auto *session = dbManager->getSession();
        if (!session)
        {
            return;
        }

        const boost::posix_time::ptime now =
            boost::posix_time::second_clock::local_time();
        const boost::gregorian::date today = now.date();
        const boost::gregorian::date yesterday =
            today - boost::gregorian::days(1);

        const std::string dayStart =
            toDateTimeString(yesterday, boost::posix_time::hours(0));
        const std::string nextDayStart =
            toDateTimeString(today, boost::posix_time::hours(0));
        const std::string recordDate =
            boost::gregorian::to_iso_extended_string(yesterday);

        session->sql("START TRANSACTION").execute();
        try
        {
            auto dailyExists = session->sql("SELECT COUNT(*) "
                                            "FROM monthlySalaryRecord "
                                            "WHERE business_date = ?")
                                   .bind(recordDate)
                                   .execute()
                                   .fetchOne();

            if (dailyExists && !dailyExists[0].isNull() &&
                dailyExists[0].get<int>() == 0)
            {
                auto dailySummary = session->sql("SELECT "
                                                 "(SELECT COALESCE(ROUND(SUM(order_totalprice)), 0) "
                                                 " FROM orders "
                                                 " WHERE created_at >= ? AND created_at < ?) AS salesCount, "
                                                 "(SELECT "
                                                 "    COALESCE(ROUND(SUM(s.total_salary / 31)), 0) + "
                                                 "    COALESCE((SELECT ROUND(SUM(om.total_price)) "
                                                 "              FROM orderMedicines AS om "
                                                 "              JOIN orders AS o ON om.order_id = o.id "
                                                 "              WHERE o.created_at >= ? AND o.created_at < ?), 0) "
                                                 " FROM salary AS s "
                                                 " JOIN users AS u ON u.id = s.user_id "
                                                 " WHERE u.account_type = 'staff') AS costCount")
                                       .bind(dayStart, nextDayStart, dayStart, nextDayStart)
                                       .execute()
                                       .fetchOne();

                const double salesCount =
                    dailySummary[0].isNull() ? 0.0 : dailySummary[0].get<double>();
                const double costCount =
                    dailySummary[1].isNull() ? 0.0 : dailySummary[1].get<double>();
                const double profitCount = salesCount - costCount;

                mysqlx::SqlResult insertDailyResult = session->sql("INSERT INTO monthlySalaryRecord "
                                                                   "(salesCount, costCount, profitCount, business_date) "
                                                                   "SELECT ?, ?, ?, ? "
                                                                   "FROM DUAL "
                                                                   "WHERE NOT EXISTS ("
                                                                   "    SELECT 1 FROM monthlySalaryRecord "
                                                                   "    WHERE business_date = ?"
                                                                   ")")
                                                       .bind(salesCount, costCount, profitCount, recordDate, recordDate)
                                                       .execute();

                if (insertDailyResult.getAffectedItemsCount() == 1)
                {
                    std::cout << "日工资汇总写入 monthlySalaryRecord 完成: "
                              << recordDate << std::endl;
                }
            }

            session->sql("COMMIT").execute();
        }
        catch (...)
        {
            rollbackTransactionQuietly(*session);
            throw;
        }

        // 每月1号00:00:00开始执行
        if (today.day() == 1)
        {
            const int targetYear = static_cast<int>(yesterday.year());
            const int targetMonth = static_cast<int>(yesterday.month().as_number());
            const boost::gregorian::date targetMonthStartDate(targetYear, targetMonth, 1);
            const boost::gregorian::date nextMonthStartDate =
                targetMonthStartDate + boost::gregorian::months(1);
            const std::string monthBusinessDate =
                boost::gregorian::to_iso_extended_string(targetMonthStartDate);

            auto monthRecordCount = session->sql("SELECT COUNT(*) "
                                                 "FROM monthlySalaryRecord "
                                                 "WHERE business_date >= ? AND business_date < ?")
                                        .bind(monthBusinessDate)
                                        .bind(boost::gregorian::to_iso_extended_string(nextMonthStartDate))
                                        .execute()
                                        .fetchOne();

            if (monthRecordCount && !monthRecordCount[0].isNull() &&
                monthRecordCount[0].get<int>() > 0)
            {
                session->sql("START TRANSACTION").execute();
                try
                {
                    mysqlx::SqlResult insertDayArchiveResult = session->sql("INSERT INTO salaryRecord "
                                                                            "(salesCount, costCount, profitCount, record_type, business_date, created_at, updated_at) "
                                                                            "SELECT msr.salesCount, msr.costCount, msr.profitCount, 'day', msr.business_date, msr.created_at, msr.updated_at "
                                                                            "FROM monthlySalaryRecord AS msr "
                                                                            "WHERE msr.business_date >= ? AND msr.business_date < ? "
                                                                            "AND NOT EXISTS ("
                                                                            "    SELECT 1 FROM salaryRecord AS sr "
                                                                            "    WHERE sr.record_type = 'day' "
                                                                            "      AND sr.business_date = msr.business_date"
                                                                            ")")
                                                                .bind(monthBusinessDate)
                                                                .bind(boost::gregorian::to_iso_extended_string(nextMonthStartDate))
                                                                .execute();

                    auto monthlySummary = session->sql("SELECT "
                                                       "COALESCE(ROUND(SUM(salesCount)), 0), "
                                                       "COALESCE(ROUND(SUM(costCount)), 0), "
                                                       "COALESCE(ROUND(SUM(profitCount)), 0) "
                                                       "FROM monthlySalaryRecord "
                                                       "WHERE business_date >= ? AND business_date < ?")
                                              .bind(monthBusinessDate)
                                              .bind(boost::gregorian::to_iso_extended_string(nextMonthStartDate))
                                              .execute()
                                              .fetchOne();

                    const double salesCount =
                        monthlySummary[0].isNull() ? 0.0 : monthlySummary[0].get<double>();
                    const double costCount =
                        monthlySummary[1].isNull() ? 0.0 : monthlySummary[1].get<double>();
                    const double profitCount =
                        monthlySummary[2].isNull() ? 0.0 : monthlySummary[2].get<double>();

                    session->sql("INSERT INTO salaryRecord "
                                 "(salesCount, costCount, profitCount, record_type, business_date) "
                                 "SELECT ?, ?, ?, 'month', ? "
                                 "FROM DUAL "
                                 "WHERE NOT EXISTS ("
                                 "    SELECT 1 FROM salaryRecord "
                                 "    WHERE record_type = 'month' AND business_date = ?"
                                 ")")
                        .bind(salesCount, costCount, profitCount, monthBusinessDate, monthBusinessDate)
                        .execute();

                    session->sql("DELETE FROM monthlySalaryRecord "
                                 "WHERE business_date >= ? AND business_date < ?")
                        .bind(monthBusinessDate)
                        .bind(boost::gregorian::to_iso_extended_string(nextMonthStartDate))
                        .execute();

                    session->sql("COMMIT").execute();

                    std::cout << "月工资明细与汇总归档完成并清理 monthlySalaryRecord: "
                              << targetYear << "-"
                              << std::setw(2) << std::setfill('0') << targetMonth
                              << "，归档日记录数: "
                              << insertDayArchiveResult.getAffectedItemsCount()
                              << std::endl;
                }
                catch (...)
                {
                    rollbackTransactionQuietly(*session);
                    throw;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "员工工资记录更新失败: " << e.what() << std::endl;
    }
}
