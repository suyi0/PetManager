#ifndef ADMINROUTES_H
#define ADMINROUTES_H

#include "../../controllers/modules/admin/adminHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"

class adminRoutes : public BaseHandler
{
public:
    // 设置管理员路由
    static void setupAdminRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};


#endif