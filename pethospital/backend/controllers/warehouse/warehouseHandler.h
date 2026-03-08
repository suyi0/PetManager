#ifndef WAREHOUSEHANDLER_H
#define WAREHOUSEHANDLER_H 

#include "../../utils/Utils.h"

class warehouseHandler : public BaseHandler {
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:
    explicit warehouseHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db) {}

    crow::response updataWarehouse(const crow::request& req);
};

#endif