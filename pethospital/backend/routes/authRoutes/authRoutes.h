#ifndef AUTHROUTES_H
#define AUTHROUTES_H 

#include "../../controllers/auth/authHandler/authHandler.h"
#include "../../middleware/authMiddleware/authMiddleware.h"


class authRoutes
{
public:
    // 设置认证路由
    static void setupAuthRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};

#endif