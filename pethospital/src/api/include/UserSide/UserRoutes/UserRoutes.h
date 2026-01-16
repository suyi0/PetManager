#pragma once


#include "../../Shared/Utils/Utils.h"
#include "UserHandler/UserHandler.h"

class UserRoutes
{
public:
    // 设置用户路由
    static void setupUserRoutes(crow::SimpleApp& app, DatabaseManagerInterface* dbManager);

};
