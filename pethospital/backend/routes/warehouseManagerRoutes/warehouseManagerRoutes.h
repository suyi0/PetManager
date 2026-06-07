#ifndef WAREHOUSEMANAGERROUTES_H
#define WAREHOUSEMANAGERROUTES_H

#include "../controllers/modules/warehouseManager/warehouseManagerHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"
#include "../../middleware/authMiddleware/authMiddleware.h"

class warehouseManagerRoutes {
public:
    static void setupwarehouseManagerRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};

#endif