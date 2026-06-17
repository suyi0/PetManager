#ifndef ADMINHANDLER_H
#define ADMINHANDLER_H

#include "../../../utils/Utils.h"
#include "../finance/financeHandler.h"

class adminHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
    financeHandler financer;
public:
    explicit adminHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)), financer(dbManager) {}

    int calculateUserCount();

    int calculateOnlineDoctorCount();

    int calculateLogsCount();

    int calculateUserLogsCount();

    int calculateSystemLogsCount();

    nlohmann::json buildHomeData(); // 构建超级管理员首页统计数据

    crow::response getHomeData(const crow::request &req); // 获取首页数据 对应 /api/admins/home-data

    crow::response getUsers(const crow::request& req);  // 获取全部用户 对应 /api/admins/users

    crow::response searchUsers(const crow::request& req, const nlohmann::json& requestBody); // 搜索用户 对应 /api/admins/users/search

    crow::response searchOnlineDoctors(const crow::request& req, const nlohmann::json& requestBody); // 搜索在线医生 对应 /api/admins/online-doctors/search

    crow::response getWorkTimeRecord(const crow::request& req); // 获取全部人的工作时间记录 对应 /api/admins/work-time-records

    // 修改医生工作时间 对应 /api/admins/doctor-work-time-changes
    crow::response changeDoctorWorkTime(const crow::request& req, int& userId, const std::string &date, const std::string &identifier);

    crow::response handleDoctorStatusAction(const crow::request& req, int& userId, bool requireDoctorId);    // 修改医生工作状态 对应 /api/admins/doctor-work-status-changes

    crow::response getLogs(const crow::request &req);   // 获取日志记录 对应 /api/admins/logs

    crow::response searchLogs(const crow::request& req, const nlohmann::json& requestBody); // 搜索日志 对应 /api/admins/logs/search

    crow::response getAllRecord(const crow::request& req, int &userId, int batch_size = 100, int offset = 1);   // 获取订单记录 对应 /api/admins/order-records
};

#endif
