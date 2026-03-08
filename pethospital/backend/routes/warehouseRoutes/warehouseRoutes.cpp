#include "warehouseRoutes.h"

void WarehouseRoutes::setupWarehouseRoutes(CrowApp& app, std::shared_ptr<DatabaseManagerInterface> dbManager) {
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
        return;

    CROW_ROUTE(app, "/updata/warehouse")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([dbManager](const crow::request &req, crow::response &res) {
            try {
                isValidUserToken(req, res, dbManager);

                if(res.code == 200)
                {
                    res.end();
                    return;
                }
                warehouseHandler warehouseHandler(dbManager);
                crow::response response = warehouseHandler.updataWarehouse(req);
                ProcessHandlerResponse(req, res, response);
            }
            catch (const std::exception &e) {
                res = ResponseHelper::system_error(req, "Internal error: " + std::string(e.what()));
            }
        });

    routes_setup = true;
}