#ifndef FINANCEHANDLER_H
#define FINANCEHANDLER_H

#include "../../../utils/Utils.h"
#include "../../../controllers/OperationLogger/OperationLogger.h"
#include "../admin/adminHandler.h"
#include "../../../utils/RoleTypeUtils/RoleTypeUtils.h"

class financeHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit financeHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    double calculateSalesCount();

    double calculateCostCount();

    crow::response getHomeData(const crow::request &req); // 获取总览数据 对应 /api/finance/getHomeData

    crow::response updateEmployeeSalary(const crow::request &req, int goalUserId); // 添加或修改员工工资并返回工资摘要 对应 /api/finance/updateEmployeeSalary

    crow::response getSalarySummary(const crow::request &req, int page = 1); // 获取员工工资列表摘要 对应 /api/finance/getSalarySummary

    crow::response getSalaryInformation(const crow::request &req, int salaryId); // 获取员工工资详情 对应 /api/finance/getSalaryInformation/<int>

    crow::response getExpenseData(const crow::request &req); // 获取工资管理页数据 对应 /api/finance/getExpenseData

};
#endif
