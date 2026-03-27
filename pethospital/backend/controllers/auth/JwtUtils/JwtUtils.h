#ifndef JWTUTILS_H
#define JWTUTILS_H

#include "../../../utils/Utils.h"

std::string url_safe_base64_encode(const std::string &data);
std::string url_safe_base64_decode(const std::string &data);
int calcDecodeLength(const std::string &b64input);
std::string get_jwt_secret();
bool verify_jwt_signature(const std::string &header, const std::string &payload,
                          const std::string &signature, const std::string &secret);


namespace JwtUtils {
    std::string createToken(int userId, const std::string &username, const int type_id, const std::string &type_name, const std::string &identifier, bool isEmail);
    std::string decodeToken(const std::string &token);

    // JWT Token解析和权限验证函数
    int getUserIdFromToken(const std::string &token);
    std::string getUserIdentifierFromToken(const std::string &token);
    bool isUserAuthorizedForOrder(int userId, int orderId, std::shared_ptr<DatabaseManagerInterface> dbManager);
    bool isUserAuthorizedForUserForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager);
    bool isUserAuthorizedForAdminForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager);
};


#endif
