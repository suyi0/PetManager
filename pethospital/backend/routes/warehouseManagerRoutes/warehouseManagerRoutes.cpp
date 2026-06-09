#include "warehouseManagerRoutes.h"
#include "../../services/logger/operationLogger.h"

void warehouseManagerRoutes::setupwarehouseManagerRoutes(
    CrowApp& app,
    std::shared_ptr<DatabaseManagerInterface> dbManager
) {
    static bool routes_setup = false;
    if (routes_setup) {
        return;
    }

    CROW_ROUTE(app, "/api/warehouse-managers/items")
        .methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)(
            [dbManager](const crow::request &req, crow::response &res) {
                int userId = -1;
                try {
                    userId = isValidWarehouseStaffToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1) {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "仓库", "上传物资");
                        return;
                    }

                    warehouseManagerHandler handler(dbManager);
                    crow::response response = handler.upload(req);
                    ProcessHandlerResponse(req, res, response);
                } catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "仓库", "上传物资", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(
                        req,
                        "Internal error: " + std::string(e.what())
                    );
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "仓库", "上传物资", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            }
        );

    CROW_ROUTE(app, "/api/warehouse-managers/items")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)(
            [dbManager](const crow::request &req, crow::response &res) {
                int userId = -1;
                try {
                    userId = isValidWarehouseStaffToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1) {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "仓库", "查询全部物资");
                        return;
                    }

                    warehouseManagerHandler handler(dbManager);
                    crow::response response = handler.selectAllData(req);
                    ProcessHandlerResponse(req, res, response);
                } catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "仓库", "查询全部物资", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(
                        req,
                        "Internal error: " + std::string(e.what())
                    );
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "仓库", "查询全部物资", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            }
        );

    CROW_ROUTE(app, "/api/warehouse-managers/items/<string>/<string>")
        .methods(crow::HTTPMethod::GET, crow::HTTPMethod::OPTIONS)(
            [dbManager](
                const crow::request &req,
                crow::response &res,
                const std::string& identifier,
                const std::string& value
            ) {
                int userId = -1;
                try {
                    userId = isValidWarehouseStaffToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1) {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "仓库", "条件查询物资");
                        return;
                    }

                    warehouseManagerHandler handler(dbManager);
                    crow::response response = handler.selectData(req, identifier, value);
                    ProcessHandlerResponse(req, res, response);
                } catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "仓库", "条件查询物资", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(
                        req,
                        "Internal error: " + std::string(e.what())
                    );
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "仓库", "条件查询物资", userId > 0 ? std::optional<int>(userId) : std::nullopt, false);
            }
        );

    CROW_ROUTE(app, "/api/warehouse-managers/items/<int>")
        .methods(crow::HTTPMethod::PATCH, crow::HTTPMethod::OPTIONS)(
            [dbManager](
                const crow::request &req,
                crow::response &res,
                const int& dataID
            ) {
                int userId = -1;
                try {
                    userId = isValidWarehouseStaffToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1) {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "仓库", "更新物资");
                        return;
                    }

                    warehouseManagerHandler handler(dbManager);
                    crow::response response = handler.updata(req, dataID);
                    ProcessHandlerResponse(req, res, response);
                } catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "仓库", "更新物资", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(
                        req,
                        "Internal error: " + std::string(e.what())
                    );
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "仓库", "更新物资", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            }
        );

    CROW_ROUTE(app, "/api/warehouse-managers/item-deletions")
        .methods(crow::HTTPMethod::DELETE, crow::HTTPMethod::OPTIONS)(
            [dbManager](
                const crow::request &req,
                crow::response &res
            ) {
                int userId = -1;
                try {
                    userId = isValidWarehouseStaffToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1) {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "仓库", "删除物资");
                        return;
                    }

                    warehouseManagerHandler handler(dbManager);
                    crow::response response = handler.deleteData(req, userId);
                    ProcessHandlerResponse(req, res, response);
                } catch (const std::exception &e) {
                    OperationLogger::LogExceptionOperation(dbManager, req, "仓库", "删除物资", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(
                        req,
                        "Internal error: " + std::string(e.what())
                    );
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "仓库", "删除物资", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            }
        );

    routes_setup = true;
}
