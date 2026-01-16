#ifndef GETORDER_H
#define GETORDER_H

#include "../../Shared/Utils/Utils.h"
#include "../../../src/DatabaseManager/DatabaseManagerInterface.h"
#include "OrderHandler/OrderHandler.h"


class OrderRoutes {
public:

    static void setupOrderRoutes(crow::SimpleApp& app, DatabaseManagerInterface* dbManager);

};

#endif
