#ifndef PERSONNELHANDLER_H
#define PERSONNELHANDLER_H

#include "../../../utils/Utils.h"

class personnelHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit personnelHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response createUser(const crow::request& req);                         // 创建用户帐号 对应 /api/admins/users

    crow::response deleteUser(const crow::request& req, int& userId);            // 软删除用户帐号 对应 /api/admins/user-deletions

    crow::response createDoctor(const crow::request& req);          // 创建医生权限 对应 /api/personnel/doctor-assignments

    crow::response deleteDoctor(const crow::request& req);          // 删除医生权限 对应 /api/personnel/doctor-removals

    crow::response createWarehouserManager(const crow::request &req);    // 创建仓库管理员权限 对应 /api/personnel/warehouse-manager-assignments

    crow::response deleteWarehouserManager(const crow::request &req);    // 删除仓库管理员权限 对应 /api/personnel/warehouse-manager-removals

};

#endif
