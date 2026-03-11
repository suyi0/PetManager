#ifndef DOCTORHANDLER_H
#define DOCTORHANDLER_H 

#include "../../utils/Utils.h"

class doctorHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit doctorHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response getDoctor(const crow::request& req);             // 获取上班医生信息  对应 /api/doctor/getDoctor

    crow::response getUserList(const crow::request &req, const std::string &name);   // 获取用户信息 对应 /api/doctor/getUserList

    crow::response onlineDoctor(const crow::request& req, int& userId);      // 医生上班 对应 /api/doctor/onlineDoctor

    crow::response offlineDoctor(const crow::request& req, int& userId);       // 医生下班 对应 /api/doctor/offlineDoctor

};


#endif
