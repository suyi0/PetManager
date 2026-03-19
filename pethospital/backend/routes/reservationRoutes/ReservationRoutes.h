#pragma once


#include "../../controllers/modules/reservation/ReservationHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"


class ReservationRoutes : public BaseHandler {
public:
    // 静态类--不需要实例化的类

    // 设置路由
    static void setupReservationRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};