#pragma once


#include "../../controllers/modules/user/userHandler/userHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"
#include "../../controllers/common/orderCommon/orderCommonHandler.h"
#include "../../controllers/common/reservationCommon/reservationCommonHandler.h"

class UserRoutes
{
public:
    // 设置用户路由
    static void setupUserRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);

};
