#ifndef BOSSHANDLER_H
#define BOSSHANDLER_H 

#include "../../../utils/Utils.h"

class bossHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:
    explicit bossHandler (std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)){}

    crow::response allocateTotalStock(const crow::request& req);    // 设置总仓股份 对应 /api/bosses/total-stock-allocations

    crow::response allocateStock(const crow::request& req);   // 修改股份分配 对应 /api/bosses/stock-allocations

    crow::response changeStock(const crow::request& req);   // 修改股份 对应 /api/bosses/stock-changes

    crow::response getStock(const crow::request& req);  // 获取股份分布 对应 /api/bosses/stocks

    // 任职审批（role-owned，v6: /api/bosses/employment-assignment-approvals）
    crow::response listEmploymentAssignmentApprovals(const crow::request &req, int operatorUserId);
    crow::response decideEmploymentAssignmentApproval(
        const crow::request &req,
        int operatorUserId,
        long long requestId,
        const nlohmann::json &body);
};
#endif
