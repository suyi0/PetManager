#pragma once


#include "../../utils/Utils.h"
#include "../../controllers/user/UserHandler/UserHandler.h"

class UserRoutes
{
public:
    // 设置用户路由
    static void setupUserRoutes(CrowApp& app, DatabaseManagerInterface* dbManager);

};
