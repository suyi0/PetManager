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

    crow::response getUsers(const crow::request& req);                           // 获取全部用户 对应 /api/admin/getUsers

    crow::response getWorkTimeRecord(const crow::request& req);                  // 获取全部人的工作时间记录 对应 /admin/getWorkTimeRecord

    // 修改医生工作时间 对应 /admin/changeDoctorWorkTime
    crow::response changeDoctorWorkTime(const crow::request& req, int& userId, const std::string &date, const std::string &identifier);

    crow::response handleDoctorStatusAction(const crow::request& req, int& userId, bool requireDoctorId);    // 修改医生工作状态 对应 /admin/changeDoctorWorkStatus

    crow::response getLogs(const crow::request &req);                     // 获取日志记录 对应 /admin/getLogs

    crow::response getSalaryManagementData(const crow::request &req);    // 获取工资管理页数据 对应 /admin/getSalaryManagementData

    crow::response changeSalary(const crow::request& req);               // 修改员工工资 对应 /admin/changeSalary
};

#endif
