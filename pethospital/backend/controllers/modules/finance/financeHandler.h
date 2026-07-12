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

    double calculateCostCount();

    double calculateCostCount(const crow::request &req);

    double calculateCostCount(int userId);

    nlohmann::json buildHomeData(); // 构建财务端首页实时统计数据

    nlohmann::json buildHomeData(const crow::request &req); // 构建带组织范围的财务端首页实时统计数据

    nlohmann::json buildHomeData(int userId); // 构建指定用户组织范围的财务端首页实时统计数据

    crow::response getHomeData(const crow::request &req); // 获取总览数据 对应 /api/finance/home-data

    crow::response updateEmployeeSalary(const crow::request &req, int goalUserId); // 添加或修改员工工资并返回工资摘要 对应 /api/finance/employee-salaries/<int>

    crow::response getSalarySummary(const crow::request &req, int page = 1); // 获取员工工资列表摘要 对应 /api/finance/salary-summaries/<int>

    crow::response getSalaryInformation(const crow::request &req, int salaryId); // 获取员工工资详情 对应 /api/finance/salary-records/<int>

    crow::response searchSalaryEmployees(const crow::request &req, const nlohmann::json &requestBody); // 搜索员工工资列表 对应 /api/finance/salary-employees/search

    crow::response getExpenseData(const crow::request &req); // 获取工资管理页数据 对应 /api/finance/expenses

    crow::response getPayrollEmployees(const crow::request &req, const nlohmann::json &requestBody);
    crow::response savePayrollEmployee(const crow::request &req, int goalUserId);
    crow::response submitPayrollReview(const crow::request &req);
    crow::response lockPayroll(const crow::request &req);

};
#endif
