#include "personnelRoutes.h"
#include "../../controllers/modules/personnel/personnelHandler.h"
#include "../../services/employment/PersonnelAccess.h"
#include "../../services/logger/operationLogger.h"
#include "../../utils/permissions/Permissions.h"

namespace
{
int requirePersonnelRead(
    const crow::request &req,
    crow::response &res,
    const std::shared_ptr<DatabaseManagerInterface> &dbManager)
{
    int userId = isValidPersonnelToken(req, res, dbManager);
    if (res.code != 200 || userId == -1)
    {
        return -1;
    }
    if (!PersonnelAccess::canReadEmployment(dbManager, userId))
    {
        res = ResponseHelper::permission_denied(req, "缺少任职管理权限");
        return -1;
    }
    return userId;
}
}

void personnelRoutes::setupPersonnelRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }

    CROW_ROUTE(app, "/api/personnel/employees/search")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = requirePersonnelRead(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "搜索员工");
                        return;
                    }
                    BaseHandler parser(dbManager);
                    auto jsonOpt = parser.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        OperationLogger::FinishLoggedRoute(dbManager, req, res, "人事", "搜索员工", userId);
                        return;
                    }
                    personnelHandler handler(dbManager);
                    crow::response response = handler.searchEmployees(req, jsonOpt.value(), userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "搜索员工", "route exception",
                                                           userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "人事", "搜索员工",
                                                   userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/personnel/employees/<int>")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int employeeId)
            {
                int userId = -1;
                try
                {
                    userId = requirePersonnelRead(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "获取员工详情");
                        return;
                    }
                    personnelHandler handler(dbManager);
                    crow::response response = handler.getEmployee(req, userId, employeeId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "获取员工详情", "route exception",
                                                           userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "人事", "获取员工详情",
                                                   userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/personnel/org/departments")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = requirePersonnelRead(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "获取部门列表");
                        return;
                    }
                    personnelHandler handler(dbManager);
                    crow::response response = handler.listDepartments(req, userId);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "获取部门列表", "route exception",
                                                           userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "人事", "获取部门列表",
                                                   userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/personnel/org/positions")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                int userId = -1;
                try
                {
                    userId = requirePersonnelRead(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "获取职位列表");
                        return;
                    }
                    nlohmann::json query = nlohmann::json::object();
                    if (const char *dept = req.url_params.get("departmentId"))
                    {
                        try
                        {
                            query["departmentId"] = std::stoi(dept);
                        }
                        catch (...)
                        {
                        }
                    }
                    if (const char *assignable = req.url_params.get("assignableOnly"))
                    {
                        query["assignableOnly"] = (std::string(assignable) == "true" || std::string(assignable) == "1");
                    }
                    personnelHandler handler(dbManager);
                    crow::response response = handler.listPositions(req, userId, query);
                    ProcessHandlerResponse(req, res, response);
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "获取职位列表", "route exception",
                                                           userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishLoggedRoute(dbManager, req, res, "人事", "获取职位列表",
                                                   userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    CROW_ROUTE(app, "/api/personnel/employees/<int>/assignment")
        .methods(crow::HTTPMethod::Put, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int employeeId)
            {
                int userId = -1;
                std::string auditPermission = Permissions::kEmploymentAssign;
                try
                {
                    userId = isValidPersonnelToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "任职变更");
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto jsonOpt = parser.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        // 由下方 FinishSensitiveRoute 收尾
                    }
                    else
                    {
                        // B16: action 必须显式；缺失/空/未知均 400，禁止按字段推断。
                        // regularize 不混入本接口。
                        if (!jsonOpt->contains("action") || jsonOpt->at("action").is_null() ||
                            (jsonOpt->at("action").is_string() && jsonOpt->at("action").get<std::string>().empty()))
                        {
                            res = ResponseHelper::validation(req, "action 必填");
                        }
                        else if (!jsonOpt->at("action").is_string())
                        {
                            res = ResponseHelper::validation(req, "action 取值不合法");
                        }
                        else
                        {
                            const std::string actionStr = jsonOpt->at("action").get<std::string>();
                            if (actionStr == "regularize")
                            {
                                res = ResponseHelper::validation(req, "转正请使用独立 regularization 接口");
                            }
                            else
                            {
                                PersonnelAccess::AssignmentAction action =
                                    PersonnelAccess::AssignmentAction::Onboard;
                                if (!PersonnelAccess::parseAssignmentAction(actionStr, action))
                                {
                                    res = ResponseHelper::validation(req, "action 取值不合法");
                                }
                                else if (!PersonnelAccess::canPerformAssignmentAction(dbManager, userId, action))
                                {
                                    res = ResponseHelper::permission_denied(req, "缺少对应任职写权限");
                                    OperationLogger::FinishAuthorizationFailure(
                                        dbManager, req, res, "人事", "任职变更");
                                    OperationLogger::FinishSensitiveRoute(
                                        dbManager, req, res, "人事", "任职变更",
                                        PersonnelAccess::permissionKeyForAction(action), userId);
                                    return;
                                }
                                else
                                {
                                    auditPermission = PersonnelAccess::permissionKeyForAction(action);
                                    personnelHandler handler(dbManager);
                                    crow::response response =
                                        handler.updateEmployeeAssignment(req, userId, employeeId, *jsonOpt);
                                    ProcessHandlerResponse(req, res, response);
                                }
                            }
                        }
                    }
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "任职变更", "route exception",
                                                           userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(dbManager, req, res, "人事", "任职变更",
                                                      auditPermission,
                                                      userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    // v6: POST /api/personnel/employees/<int>/regularization（无 -requests 别名）
    CROW_ROUTE(app, "/api/personnel/employees/<int>/regularization")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int employeeId)
            {
                int userId = -1;
                try
                {
                    userId = isValidPersonnelToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "转正");
                        return;
                    }
                    if (!PersonnelAccess::canPerformAssignmentAction(
                            dbManager, userId, PersonnelAccess::AssignmentAction::Regularize))
                    {
                        res = ResponseHelper::permission_denied(req, "缺少转正权限");
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "转正");
                        OperationLogger::FinishSensitiveRoute(
                            dbManager, req, res, "人事", "转正",
                            Permissions::kEmploymentRegularize, userId);
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto jsonOpt = parser.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        // 由 FinishSensitiveRoute 收尾
                    }
                    else
                    {
                        personnelHandler handler(dbManager);
                        crow::response response =
                            handler.createRegularization(req, userId, employeeId, *jsonOpt);
                        ProcessHandlerResponse(req, res, response);
                    }
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "转正", "route exception",
                                                           userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(
                    dbManager, req, res, "人事", "转正",
                    Permissions::kEmploymentRegularize,
                    userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    // v6: POST /api/personnel/employees/<int>/offboarding（无 -requests 别名）
    CROW_ROUTE(app, "/api/personnel/employees/<int>/offboarding")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int employeeId)
            {
                int userId = -1;
                try
                {
                    userId = isValidPersonnelToken(req, res, dbManager);
                    if (res.code != 200 || userId == -1)
                    {
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "离职");
                        return;
                    }
                    if (!PersonnelAccess::canPerformAssignmentAction(
                            dbManager, userId, PersonnelAccess::AssignmentAction::Offboard))
                    {
                        res = ResponseHelper::permission_denied(req, "缺少离职权限");
                        OperationLogger::FinishAuthorizationFailure(dbManager, req, res, "人事", "离职");
                        OperationLogger::FinishSensitiveRoute(
                            dbManager, req, res, "人事", "离职",
                            Permissions::kEmploymentOffboard, userId);
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto jsonOpt = parser.parseJson(req, res);
                    if (!jsonOpt)
                    {
                        // 由 FinishSensitiveRoute 收尾
                    }
                    else
                    {
                        personnelHandler handler(dbManager);
                        crow::response response =
                            handler.createOffboarding(req, userId, employeeId, *jsonOpt);
                        ProcessHandlerResponse(req, res, response);
                    }
                }
                catch (const std::exception &)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "人事", "离职", "route exception",
                                                           userId > 0 ? std::optional<int>(userId) : std::nullopt);
                    res = ResponseHelper::system_error(req);
                }
                OperationLogger::FinishSensitiveRoute(
                    dbManager, req, res, "人事", "离职",
                    Permissions::kEmploymentOffboard,
                    userId > 0 ? std::optional<int>(userId) : std::nullopt);
            });

    routes_setup = true;
}
