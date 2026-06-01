#ifndef FINANCEHANDLER_H
#define FINANCEHANDLER_H 

#include "../../../utils/Utils.h"

class financeHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:
    explicit financeHandler (std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)){}

    double calculateSalesCount();

    double calculateCostCount();

    crow::response homePageGetData(const crow::request &req);            // 获取总览数据 对应 /api/finance/homePageGetData 与 /api/admin/homePageGetData

};
#endif
