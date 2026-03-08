#pragma once


#include "../../controllers/user/UserHandler/UserHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"

class UserRoutes
{
public:
    // 设置用户路由
    static void setupUserRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);

};
