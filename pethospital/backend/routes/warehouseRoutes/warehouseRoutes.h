#ifndef WAREHOUSEROUTES_H
#define WAREHOUSEROUTES_H

#include "../controllers/warehouse/warehouseHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"

class WarehouseRoutes {
public:
    static void setupWarehouseRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};

#endif