#ifndef ORDERHANDLER_H
#define ORDERHANDLER_H 

#include "../../../../src/DatabaseManager/DatabaseManagerInterface.h"
#include "../../../Shared/Utils/Utils.h"

class OrderHandler {
private:
    DatabaseManagerInterface* dbManager;
    
public:
    explicit OrderHandler(DatabaseManagerInterface* dbManager) : dbManager(dbManager) {}

    crow::response getOrder(const crow::request& req); // 对应 /api/order/getrecord
};

#endif