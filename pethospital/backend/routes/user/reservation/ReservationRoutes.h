#pragma once
#include <mysqlx/xdevapi.h>

#include "nlohmann/json.hpp"
#include "../UserRoutes.h"
#include "../../../utils/Utils.h"
#include "../../../database/DatabaseManager.h"
#include "../../../controllers/reservation/user/ReservationHandler.h"

class ReservationRoutes {
public:
    // 静态类--不需要实例化的类

    // 设置路由
    static void setupReservationRoutes(CrowApp& app, DatabaseManagerInterface *dbManager);
};