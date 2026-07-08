#ifndef JWTUTILS_H
#define JWTUTILS_H

#include "../../../utils/Utils.h"
#include <optional>

std::string url_safe_base64_encode(const std::string &data);
std::string url_safe_base64_decode(const std::string &data);
int calcDecodeLength(const std::string &b64input);
std::string get_jwt_secret();
bool verify_jwt_signature(const std::string &header, const std::string &payload,
                          const std::string &signature, const std::string &secret);


namespace JwtUtils {
    struct TokenClaims {
        int userId;
        int typeId;
        std::string typeName;
        std::string identifier;
        bool isEmailLogin;
        int sessionVersion;
    };

    struct UpdateTicketClaims {
        int userId;
        std::string data;
        std::string identifier;
    };

    // managementSession 由调用方按当前权限算好传入（RbacService::userHasManagementAccess），
    // 仅决定 token TTL（管理端短时效）。jwtUtils 不自查库，避免把 DB 依赖下沉到低层工具。
    std::string createToken(int userId, const int type_id, const std::string &type_name, const std::string &identifier, bool isEmail, bool managementSession, int sessionVersion = -1);
    std::string createUpdateTicket(int userId, const std::string &date, const std::string &identifier);
    std::optional<UpdateTicketClaims> getUpdateTicketClaims(const std::string &ticket, const std::string &data, const std::string &identifier);

    // JWT Token解析和权限验证函数
    std::optional<TokenClaims> getTokenClaims(const std::string &token);
    int getUserIdFromToken(const std::string &token);
    bool isUserAuthorizedForOrder(int userId, int orderId, std::shared_ptr<DatabaseManagerInterface> dbManager);
    bool isUserAuthorizedForUserForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager);
    bool isUserAuthorizedForAdminForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager);
    bool isUserAuthorizedForSuperAdminPortal(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager);
    bool isUserAuthorizedForFinancePortal(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager);
    bool isUserAuthorizedForBossPortal(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager);
    bool isUserAuthorizedForPermission(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &permissionKey);
    bool isUserAuthorizedForPersonnelForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager);
    bool isUserAuthorizedForMedicalStaffForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager);
    bool isUserAuthorizedForWarehouseStaffForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager);
};


#endif
