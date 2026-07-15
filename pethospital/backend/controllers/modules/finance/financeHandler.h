#ifndef FINANCEHANDLER_H
#define FINANCEHANDLER_H

#include "../../../utils/Utils.h"
#include "../../../services/logger/operationLogger.h"

class financeHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit financeHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    double calculateSalesCount();

    double calculateSalesCount(const std::string &orderScopeFilter); // 按订单部门范围过滤的当日销售额

    double calculateCostCount();

    double calculateCostCount(const crow::request &req);

    double calculateCostCount(int userId);

    nlohmann::json buildHomeData(); // 构建财务端首页实时统计数据

    nlohmann::json buildHomeData(const crow::request &req); // 构建带组织范围的财务端首页实时统计数据

    nlohmann::json buildHomeData(int userId); // 构建指定用户组织范围的财务端首页实时统计数据

    crow::response getHomeData(const crow::request &req); // 获取总览数据 对应 /api/finance/home-data


    crow::response getSalarySummary(const crow::request &req, int page = 1); // 获取员工工资列表摘要 对应 /api/finance/salary-summaries/<int>

    crow::response getSalaryInformation(const crow::request &req, int salaryId); // 获取员工工资详情 对应 /api/finance/salary-records/<int>

    crow::response searchSalaryEmployees(const crow::request &req, const nlohmann::json &requestBody); // 搜索员工工资列表 对应 /api/finance/salary-employees/search

    crow::response getExpenseData(const crow::request &req); // 获取工资管理页数据 对应 /api/finance/expenses

    crow::response getPayrollEmployees(const crow::request &req, const nlohmann::json &requestBody);
    crow::response savePayrollEmployee(const crow::request &req, int goalUserId);
    // 旧 POST 旁路已退役：不得再写 salaryProfile 金额；请走 compensation 激活闭环。
    crow::response saveSalaryProfile(const crow::request &req, int goalUserId);
    crow::response getSalaryChangeHistory(const crow::request &req, long long salaryId);
    crow::response getSalaryProfile(const crow::request &req, int goalUserId);
    // v6 财务激活（精确 salary-profile:activate）
    crow::response listCompensationActivations(const crow::request &req, int operatorUserId);
    crow::response confirmCompensationActivation(
        const crow::request &req, int operatorUserId, long long proposalId, const nlohmann::json &body);
    crow::response returnCompensationActivation(
        const crow::request &req, int operatorUserId, long long proposalId, const nlohmann::json &body);
    crow::response reviewPayrollEmployee(const crow::request &req, int goalUserId);
    crow::response submitPayrollReview(const crow::request &req);
    crow::response supervisorReviewPayroll(const crow::request &req);
    crow::response lockPayroll(const crow::request &req);
    crow::response createPayrollRevision(const crow::request &req);
    crow::response getPayrollAuditEvents(const crow::request &req);

};
#endif
