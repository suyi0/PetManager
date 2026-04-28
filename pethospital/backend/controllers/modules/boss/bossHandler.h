#ifndef BOSSHANDLER_H
#define BOSSHANDLER_H 

#include "../../../utils/Utils.h"

class bossHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:
    explicit bossHandler (std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)){}

    crow::response allocateTotalStock(const crow::request& req);    // 设置总仓股份 对应 /boss/allocateTotalStock

    crow::response allocateStock(const crow::request& req);   // 修改股份分配 对应 /boss/allocateStock

    crow::response changeStock(const crow::request& req);   // 修改股份 对应 /boss/changeStock

    crow::response getStock(const crow::request& req);  // 获取股份分布 对应 /boss/getStock
};
#endif