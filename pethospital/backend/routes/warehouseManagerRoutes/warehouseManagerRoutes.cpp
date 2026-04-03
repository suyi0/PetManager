#include "warehouseManagerRoutes.h"

void warehouseManagerRoutes::setupwarehouseManagerRoutes(
    CrowApp& app,
    std::shared_ptr<DatabaseManagerInterface> dbManager
) {
    static bool routes_setup = false;
    if (routes_setup) {
        return;
    }

    CROW_ROUTE(app, "/api/warehouseManager/upload")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)(
            [dbManager](const crow::request &req, crow::response &res) {
                try {
                    isValidUserToken(req, res, dbManager);
                    if (res.code == 200) {
                        res.end();
                        return;
                    }

                    warehouseManagerHandler handler(dbManager);
                    crow::response response = handler.upload(req);
                    ProcessHandlerResponse(req, res, response);
                } catch (const std::exception &e) {
                    res = ResponseHelper::system_error(
                        req,
                        "Internal error: " + std::string(e.what())
                    );
                }
            }
        );

    CROW_ROUTE(app, "/api/warehouseManager/select")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)(
            [dbManager](const crow::request &req, crow::response &res) {
                try {
                    isValidUserToken(req, res, dbManager);
                    if (res.code == 200) {
                        res.end();
                        return;
                    }

                    warehouseManagerHandler handler(dbManager);
                    crow::response response = handler.selectAllData(req);
                    ProcessHandlerResponse(req, res, response);
                } catch (const std::exception &e) {
                    res = ResponseHelper::system_error(
                        req,
                        "Internal error: " + std::string(e.what())
                    );
                }
            }
        );

    CROW_ROUTE(app, "/api/warehouseManager/select/<string>/<string>")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)(
            [dbManager](
                const crow::request &req,
                crow::response &res,
                const std::string& identifier,
                const std::string& value
            ) {
                try {
                    isValidUserToken(req, res, dbManager);
                    if (res.code == 200) {
                        res.end();
                        return;
                    }

                    warehouseManagerHandler handler(dbManager);
                    crow::response response = handler.selectData(req, identifier, value);
                    ProcessHandlerResponse(req, res, response);
                } catch (const std::exception &e) {
                    res = ResponseHelper::system_error(
                        req,
                        "Internal error: " + std::string(e.what())
                    );
                }
            }
        );

    CROW_ROUTE(app, "/api/warehouseManager/updata/<int>")
        .methods(crow::HTTPMethod::PATCH, crow::HTTPMethod::OPTIONS)(
            [dbManager](
                const crow::request &req,
                crow::response &res,
                const int& dataID
            ) {
                try {
                    isValidUserToken(req, res, dbManager);
                    if (res.code == 200) {
                        res.end();
                        return;
                    }

                    warehouseManagerHandler handler(dbManager);
                    crow::response response = handler.updata(req, dataID);
                    ProcessHandlerResponse(req, res, response);
                } catch (const std::exception &e) {
                    res = ResponseHelper::system_error(
                        req,
                        "Internal error: " + std::string(e.what())
                    );
                }
            }
        );

    CROW_ROUTE(app, "/api/warehouseManager/delete")
        .methods(crow::HTTPMethod::DELETE, crow::HTTPMethod::OPTIONS)(
            [dbManager](
                const crow::request &req,
                crow::response &res
            ) {
                try {
                    isValidUserToken(req, res, dbManager);
                    if (res.code == 200) {
                        res.end();
                        return;
                    }

                    warehouseManagerHandler handler(dbManager);
                    crow::response response = handler.deleteData(req);
                    ProcessHandlerResponse(req, res, response);
                } catch (const std::exception &e) {
                    res = ResponseHelper::system_error(
                        req,
                        "Internal error: " + std::string(e.what())
                    );
                }
            }
        );

    routes_setup = true;
}
