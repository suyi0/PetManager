#ifndef ADMINHANDLER_H
#define ADMINHANDLER_H

#include "../../../utils/Utils.h"

class adminHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:

    int getUserCount();

    int getOnlineDoctorCount();

    int getLogsCount();
    explicit adminHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)){}

    crow::response getUsers(const crow::request& req);                           // 获取全部用户 对应 /api/admin/getUsers

    crow::response getWorkTimeRecord(const crow::request& req);                  // 获取全部人的工作时间记录 对应 /admin/getWorkTimeRecord

    crow::response createUser(const crow::request& req);                         // 创建用户帐号 对应 /admin/createUser

    crow::response deleteUser(const crow::request& req, int& userId);            // 删除用户帐号 对应 /admin/deleteUser

    crow::response createDoctor(const crow::request& req);          // 创建医生权限 对应 /admin/createDoctor

    crow::response deleteDoctor(const crow::request& req);          // 删除医生权限 对应 /admin/deleteDoctor

    crow::response createWarehouserManager(const crow::request &req);    // 创建仓库管理员权限 对应 /admin/createWarehouserManager

    crow::response deleteWarehouserManager(const crow::request &req);    // 删除仓库管理员权限 对应 /admin/deleteWarehouserManager

    // 修改医生工作时间 对应 /admin/changeDoctorWorkTime
    crow::response changeDoctorWorkTime(const crow::request& req, int& userId, const std::string &date, const std::string &identifier);

    crow::response handleDoctorStatusAction(const crow::request& req, int& userId, bool requireDoctorId);    // 修改医生工作状态 对应 /admin/changeDoctorWorkStatus

    crow::response getLogs(const crow::request &req);                     // 获取日志记录 对应 /admin/getLogs

    crow::response homePageGetData(const crow::request &req);                     // 获取总览数据 对应 /admin/homeGetData
};

#endif
