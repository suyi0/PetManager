#include "dbSessionGuardMiddleware.h"

#include "../../database/DatabaseManager.h"

void DbSessionGuardMiddleware::before_handle(crow::request &req, crow::response &res, context &ctx)
{
    (void)req;
    (void)res;
    (void)ctx;
    DatabaseManager::endOfRequestCleanup();
}

void DbSessionGuardMiddleware::after_handle(crow::request &req, crow::response &res, context &ctx)
{
    (void)req;
    (void)res;
    (void)ctx;
    DatabaseManager::endOfRequestCleanup();
}
