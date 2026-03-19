#ifndef ADMINHANDLER_H
#define ADMINHANDLER_H

#include "../../../utils/Utils.h"

class adminHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:
    explicit adminHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)){}

    crow::response getWorkTimeRecord(const crow::request& req);                  // 获取全部人的工作时间记录 对应 /admin/getWorkTimeRecord

    crow::response createUser(const crow::request& req);                         // 创建用户帐号 对应 /admin/createUser

    crow::response deleteUser(const crow::request& req, int& userId);            // 删除用户帐号 对应 /admin/deleteUser

    crow::response createDoctor(const crow::request& req);          // 创建医生权限 对应 /admin/createDoctor

    crow::response deleteDoctor(const crow::request& req);          // 删除医生权限 对应 /admin/deleteDoctor

    // 修改医生工作时间 对应 /admin/changeDoctorWorkTime
    crow::response changeDoctorWorkTime(const crow::request& req, int& userId, std::string &date, std::string &identifier);

};

#endif
