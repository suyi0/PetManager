#pragma once
#include <mysqlx/xdevapi.h>

#include "nlohmann/json.hpp"
#include "../UserRoutes/UserRoutes.h"
#include "../../Shared/Utils/Utils.h"
#include "../../../src/DatabaseManager/DatabaseManager.h"
#include "ReservationHandler/ReservationHandler.h"

class ReservationRoutes {
public:
    // 静态类--不需要实例化的类

    // 设置路由
    static void setupReservationRoutes(CrowApp& app, DatabaseManagerInterface *dbManager);
};