#include "authMiddleware.h"
#include "../../services/auth/AuthSessionStore.h"

namespace
{
    enum class TokenValidationScope
    {
        User,
        Management,
        SuperAdminPortal,
        FinancePortal,
        BossPortal,
        Personnel,
        MedicalStaff,
        WarehouseStaff
    };

    enum class AuthorizationFailureResponse
    {
        Unauthorized,
        PermissionDenied
    };

    template <typename Authorizer>
    int validateToken(const crow::request &req,
                      crow::response &res,
                      std::shared_ptr<DatabaseManagerInterface> dbManager,
                      Authorizer authorizer,
                      AuthorizationFailureResponse authorizationFailureResponse = AuthorizationFailureResponse::Unauthorized)
    {
        // 从请求头取 Bearer token
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
        {
            res = ResponseHelper::unauthorized(req, "Missing or invalid token");
            return -1;
        }

        // 判断 token 是否为空
        std::string token = authHeader.substr(7);
        if (token.empty())
        {
            res = ResponseHelper::unauthorized(req, "Empty token provided");
            return -1;
        }

        // 解析 JWT claims
        auto claims = JwtUtils::getTokenClaims(token);
        if (!claims || claims->userId <= 0)
        {
            res = ResponseHelper::unauthorized(req, "Invalid or expired token");
            return -1;
        }

        if (!AuthSessionStore::isSessionCurrent(claims->userId, claims->typeName, claims->sessionVersion))
        {
            res = ResponseHelper::unauthorized(req, "Session expired");
            return -1;
        }

        // 检查数据库连接
        if (!dbManager || !dbManager->getSession() || !dbManager->getSchema())
        {
            res = ResponseHelper::system_error(req, "Database connection unavailable");
            return -1;
        }

        // 最后调用外部传进来的 authorizer(...) 做“权限判断”
        std::string identifier = claims->identifier;
        const bool isAuthorized = authorizer(*claims, identifier);
        if (!isAuthorized)
        {
            res = authorizationFailureResponse == AuthorizationFailureResponse::PermissionDenied
                      ? ResponseHelper::permission_denied(req, "用户无权限进行此操作")
                      : ResponseHelper::unauthorized(req, "用户无权限进行此操作");
            return -1;
        }

        return claims->userId;
    }

    // 统一的Token验证函数，根据不同的权限范围进行验证
    int validateTokenWithScope(const crow::request &req,
                               crow::response &res,
                               std::shared_ptr<DatabaseManagerInterface> dbManager,
                               TokenValidationScope scope)
    {
        return validateToken(req, res, dbManager, [&](const JwtUtils::TokenClaims &claims, std::string &identifier)
                             {
            if (scope == TokenValidationScope::Management)
            {
                return JwtUtils::isUserAuthorizedForAdminForm(claims.userId, identifier, claims.isEmailLogin, dbManager);
            }
            if (scope == TokenValidationScope::SuperAdminPortal)
            {
                return JwtUtils::isUserAuthorizedForSuperAdminPortal(claims.userId, identifier, claims.isEmailLogin, dbManager);
            }
            if (scope == TokenValidationScope::FinancePortal)
            {
                return JwtUtils::isUserAuthorizedForFinancePortal(claims.userId, identifier, claims.isEmailLogin, dbManager);
            }
            if (scope == TokenValidationScope::BossPortal)
            {
                return JwtUtils::isUserAuthorizedForBossPortal(claims.userId, identifier, claims.isEmailLogin, dbManager);
            }
            if (scope == TokenValidationScope::Personnel)
            {
                return JwtUtils::isUserAuthorizedForPersonnelForm(claims.userId, identifier, claims.isEmailLogin, dbManager);
            }
            if (scope == TokenValidationScope::MedicalStaff)
            {
                return JwtUtils::isUserAuthorizedForMedicalStaffForm(claims.userId, identifier, claims.isEmailLogin, dbManager);
            }
            if (scope == TokenValidationScope::WarehouseStaff)
            {
                return JwtUtils::isUserAuthorizedForWarehouseStaffForm(claims.userId, identifier, claims.isEmailLogin, dbManager);
            }
            return JwtUtils::isUserAuthorizedForUserForm(claims.userId, identifier, claims.isEmailLogin, dbManager); });
    }
}

// 验证用户token
int isValidUserToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    return validateTokenWithScope(req, res, dbManager, TokenValidationScope::User);
}

// 验证用户的订单token
int isValidUserorderToken(const crow::request &req, crow::response &res, int &orderId, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    // 1. 验证JWT token
    std::string authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
    {
        res = ResponseHelper::unauthorized(req, "Missing or invalid token");
        return -1;
    }

    std::string token = authHeader.substr(7); // 移除 "Bearer " 前缀

    // 2. 基本token验证
    if (token.empty())
    {
        res = ResponseHelper::unauthorized(req, "Empty token provided");
        return -1;
    }

    // 3. 解析token获取用户信息
    int userId = JwtUtils::getUserIdFromToken(token);
    if (userId <= 0)
    {
        std::string errorMsg;
        switch (userId)
        {
        case -1:
            errorMsg = "Invalid token format";
            break;
        case -2:
            errorMsg = "Token signature invalid";
            break;
        case -3:
            errorMsg = "Token expired";
            break;
        case -4:
            errorMsg = "User not found";
            break;
        default:
            errorMsg = "Token parsing error";
            break;
        }
        res = ResponseHelper::unauthorized(req, errorMsg);
        return -1;
    }

    // 3.1 会话失效校验：与其它 token 入口一致，补齐"所有 bearer token 入口都校验 session-version"的不变量。
    // 订单 token 多为非管理角色，此校验通常是 no-op，但管理角色访问订单时同样不会用失效会话。
    auto orderClaims = JwtUtils::getTokenClaims(token);
    if (orderClaims && !AuthSessionStore::isSessionCurrent(orderClaims->userId, orderClaims->typeName, orderClaims->sessionVersion))
    {
        res = ResponseHelper::unauthorized(req, "Session expired");
        return -1;
    }

    // 4. 验证数据库连接
    if (!dbManager || !dbManager->getSession() || !dbManager->getSchema())
    {
        res = ResponseHelper::system_error(req, "Database connection unavailable");
        return -1;
    }

    // 5. 验证用户权限
    if (!JwtUtils::isUserAuthorizedForOrder(userId, orderId, dbManager))
    {
        res = ResponseHelper::notFound(req, "Order not found");
        return -1;
    }

    return userId;
}

// 验证管理端 token。
int isValidManagementToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    return validateTokenWithScope(req, res, dbManager, TokenValidationScope::Management);
}

// 验证指定功能权限 token。
int isValidPermissionToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &permissionKey)
{
    return validateToken(req, res, dbManager, [&](const JwtUtils::TokenClaims &claims, std::string &identifier)
                         {
        return JwtUtils::isUserAuthorizedForPermission(
            claims.userId,
            identifier,
            claims.isEmailLogin,
            dbManager,
            permissionKey); },
                         AuthorizationFailureResponse::PermissionDenied);
}

// 验证超级管理员门户 token。
int isValidSuperAdminPortalToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    return validateTokenWithScope(req, res, dbManager, TokenValidationScope::SuperAdminPortal);
}

// 验证财务门户 token。
int isValidFinancePortalToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    return validateTokenWithScope(req, res, dbManager, TokenValidationScope::FinancePortal);
}

// 验证总裁门户 token。
int isValidBossPortalToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    return validateTokenWithScope(req, res, dbManager, TokenValidationScope::BossPortal);
}

// 验证人事部门token
int isValidPersonnelToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    return validateTokenWithScope(req, res, dbManager, TokenValidationScope::Personnel);
}

// 验证医疗端 token。
int isValidMedicalStaffToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    return validateTokenWithScope(req, res, dbManager, TokenValidationScope::MedicalStaff);
}

// 验证仓储端 token。
int isValidWarehouseStaffToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    return validateTokenWithScope(req, res, dbManager, TokenValidationScope::WarehouseStaff);
}
