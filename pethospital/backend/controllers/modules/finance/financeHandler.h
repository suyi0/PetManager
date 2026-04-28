#ifndef FINANCEHANDLER_H
#define FINANCEHANDLER_H 

#include "../../../utils/Utils.h"
#include "../admin/adminHandler.h"

class financeHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
    adminHandler adminer;
public:
    explicit financeHandler (std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)), adminer(dbManager){}

    double calculateSalesCount();

    double calculateCostCount();

    crow::response homePageGetData(const crow::request &req);            // 获取总览数据 对应 /finance/homeGetData

};
#endif