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
    // v6: POST .../regularization、.../offboarding
    crow::response createRegularization(const crow::request &req, int operatorUserId, int employeeId, const nlohmann::json &body);
    crow::response createOffboarding(const crow::request &req, int operatorUserId, int employeeId, const nlohmann::json &body);

    // v6 薪酬提案（compensation:propose / compensation:reassign-case）
    crow::response listCompensationProposals(const crow::request &req, int operatorUserId);
    crow::response createCompensationProposal(const crow::request &req, int operatorUserId, const nlohmann::json &body);
    crow::response updateCompensationProposal(
        const crow::request &req, int operatorUserId, long long proposalId, const nlohmann::json &body);
    crow::response submitCompensationProposal(
        const crow::request &req, int operatorUserId, long long proposalId, const nlohmann::json &body);
    crow::response reassignCompensationProposal(
        const crow::request &req, int operatorUserId, long long proposalId, const nlohmann::json &body);
};

#endif
