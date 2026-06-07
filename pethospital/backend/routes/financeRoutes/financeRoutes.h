#ifndef FINANCEROUTES_H
#define FINANCEROUTES_H

#include "../../controllers/modules/finance/financeHandler.h"
#include "../../controllers/auth/authHandler/authHandler.h"
#include "../../middleware/authMiddleware/authMiddleware.h"

class financeRoutes : public BaseHandler
{
public:
    static void setupFinanceRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager);
};

#endif