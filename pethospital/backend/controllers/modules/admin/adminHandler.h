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

    crow::response getWorkTimeRecord(const crow::request& req);                  // 获取全部人的工作时间记录 对应 /api/admin/getWorkTimeRecord

    // 修改医生工作时间 对应 /api/admin/changeDoctorWorkTime
    crow::response changeDoctorWorkTime(const crow::request& req, int& userId, const std::string &date, const std::string &identifier);

    crow::response handleDoctorStatusAction(const crow::request& req, int& userId, bool requireDoctorId);    // 修改医生工作状态 对应 /api/admin/changeDoctorWorkStatus

    crow::response getLogs(const crow::request &req);                     // 获取日志记录 对应 /api/admin/getLogs

    crow::response getAllRecord(const crow::request& req, int &userId, int batch_size = 100, int offset = 1);                       // 获取订单记录 对应 /api/admin/order/getAllRecord
};

#endif
