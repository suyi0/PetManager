#pragma once
#include <mysqlx/xdevapi.h>

#include "nlohmann/json.hpp"
#include "User/User.h"
#include "UserEncrypt/Encrypt.h"
#include "Verification/Verification.h"
#include "../include/GetAddress.h"
#include "../include/Utils.h"

// 添加全局数据库会话变量的外部声明
extern mysqlx::Session *g_db_session; // 全局数据库会话指针
extern mysqlx::Schema *g_database; // 全局数据库指针

class UserRoutes
{
public:
    // 设置用户路由
    static void setupUserRoutes(crow::SimpleApp& app);

    // 单例模式
    static UserRoutes &instance();
};
