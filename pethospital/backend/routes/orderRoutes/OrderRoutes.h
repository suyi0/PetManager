#ifndef GETORDER_H
#define GETORDER_H

#include "../../controllers/order/OrderHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"


class OrderRoutes {
public:

    static void setupOrderRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);

};

#endif
