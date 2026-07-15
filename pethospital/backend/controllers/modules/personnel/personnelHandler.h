#ifndef PERSONNELHANDLER_H
#define PERSONNELHANDLER_H

#include "../../../utils/Utils.h"

class personnelHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit personnelHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response createUser(const crow::request &req);

    crow::response deleteUser(const crow::request &req, int &userId);

    // 人事自有员工搜索 / 详情 / 组织只读 / 任职写
    crow::response searchEmployees(const crow::request &req, const nlohmann::json &body, int operatorUserId);
    crow::response getEmployee(const crow::request &req, int operatorUserId, int employeeId);
    crow::response listDepartments(const crow::request &req, int operatorUserId);
    crow::response listPositions(const crow::request &req, int operatorUserId, const nlohmann::json &query);
    crow::response updateEmployeeAssignment(const crow::request &req, int operatorUserId, int employeeId, const nlohmann::json &body);
};

#endif
