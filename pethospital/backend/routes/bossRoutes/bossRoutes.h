#ifndef BOSSROUTES_H
#define BOSSSROUTES_H

#include "../../controllers/modules/boss/bossHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"

class bossRoutes : public BaseHandler
{
public:
    static void setupBossRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};
#endif