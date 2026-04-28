#ifndef PERSONNELROUTES_H
#define PERSONNELROUTES_H 

#include "../../controllers/modules/personnel/personnelHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"

class personnelRoutes : public BaseHandler
{
public:
    static void setupPersonnelRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};
#endif