#ifndef GETORDER_H
#define GETORDER_H

#include "../../../utils/Utils.h"
#include "../../../database/DatabaseManagerInterface.h"
#include "../../../controllers/order/user/OrderHandler.h"


class OrderRoutes {
public:

    static void setupOrderRoutes(CrowApp& app, DatabaseManagerInterface* dbManager);

};

#endif
