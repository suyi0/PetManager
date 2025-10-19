#pragma once
#include <mysqlx/xdevapi.h>

#include "nlohmann/json.hpp"
#include "../UserRoutes/UserRoutes.h"
#include "../include/Utils.h"
#include "Reservation/Reservate.h"

// 添加全局数据库会话变量的外部声明
extern mysqlx::Session *g_db_session; // 全局数据库会话指针
extern mysqlx::Schema *g_database; // 全局数据库指针

class ReservationRoutes {
public:
    // 设置路由
    static void setupReservationRoutes(crow::SimpleApp& app);

    // 单例模式
    static ReservationRoutes &instance();
};