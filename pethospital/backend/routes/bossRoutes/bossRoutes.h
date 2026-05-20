#ifndef BOSSROUTES_H
#define BOSSROUTES_H

#include "../../controllers/modules/boss/bossHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"

class bossRoutes : public BaseHandler
{
public:
    static void setupBossRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};
#endif
