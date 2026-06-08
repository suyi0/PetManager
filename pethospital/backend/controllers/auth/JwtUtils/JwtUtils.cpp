#include "jwtUtils.h"
#include "../../../utils/roleTypeUtils/roleTypeUtils.h"
#include <cstring>

namespace
{
    constexpr time_t kManagementTokenTtlSeconds = 1800;  // 管理门户 30 分钟
    constexpr time_t kDefaultTokenTtlSeconds = 604800;   // 普通门户 7 天
    constexpr time_t kEmailChangeTicketTtlSeconds = 300; // 修改邮箱凭证 5 分钟

    struct UserAuthTarget
    {
        int userId;
        std::string roleName;
    };

    // 解析并验证JWT Token的有效性，返回解析后的payload数据
    std::optional<nlohmann::json> parseValidatedTokenPayload(const std::string &token)
    {
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);

        if (first_dot == std::string::npos || second_dot == std::string::npos)
        {
            return std::nullopt;
        }

        std::string header_encoded = token.substr(0, first_dot);
        std::string payload_encoded = token.substr(first_dot + 1, second_dot - first_dot - 1);
        std::string signature_encoded = token.substr(second_dot + 1);

        std::string secret_key = get_jwt_secret();
        if (!verify_jwt_signature(header_encoded, payload_encoded, signature_encoded, secret_key))
        {
            return std::nullopt;
        }

        // 解码payload，解析JSON数据为nlohmann::json对象
        std::string payload_decoded = url_safe_base64_decode(payload_encoded);
        nlohmann::json payload_json = nlohmann::json::parse(payload_decoded);

        time_t now = time(nullptr);
        if (payload_json.contains("exp") && payload_json["exp"].get<time_t>() < now)
        {
            return std::nullopt;
        }

        return payload_json;
    }

    std::optional<UserAuthTarget> getUserAuthTarget(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        std::string &identifier,
        bool isEmail)
    {
        mysqlx::RowResult result;
        if (isEmail)
        {
            result = dbManager->getSession()
                         ->sql("SELECT u.id, t.type FROM users AS u "
                               "JOIN types AS t ON u.type_id = t.id "
                               "WHERE u.email = ? AND u.is_deleted = 0")
                         .bind(identifier)
                         .execute();
        }
        else
        {
            result = dbManager->getSession()
                         ->sql("SELECT u.id, t.type FROM users AS u "
                               "JOIN types AS t ON u.type_id = t.id "
                               "WHERE u.phone = ? AND u.is_deleted = 0")
                         .bind(identifier)
                         .execute();
        }

        if (result.count() == 0)
        {
            return std::nullopt;
        }

        auto row = result.fetchOne();
        return UserAuthTarget{
            row[0].get<int>(),
            row[1].get<std::string>()};
    }

    std::optional<UserAuthTarget> getUserAuthTargetById(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        int userId)
    {
        if (userId <= 0)
        {
            return std::nullopt;
        }

        auto result = dbManager->getSession()
                          ->sql("SELECT u.id, t.type FROM users AS u "
                                "JOIN types AS t ON u.type_id = t.id "
                                "WHERE u.id = ? AND u.is_deleted = 0")
                          .bind(userId)
                          .execute();

        if (result.count() == 0)
        {
            return std::nullopt;
        }

        auto row = result.fetchOne();
        return UserAuthTarget{
            row[0].get<int>(),
            row[1].get<std::string>()};
    }

    // 创建JWT凭证
    std::string createSignedJwt(const nlohmann::json &payload_json)
    {
        std::string header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
        std::string encoded_header = url_safe_base64_encode(header);
        std::string encoded_payload = url_safe_base64_encode(payload_json.dump());
        std::string signature_data = encoded_header + "." + encoded_payload;
        std::string secret_key = get_jwt_secret();

        unsigned char signature[EVP_MAX_MD_SIZE];
        unsigned int signature_len;

        if (HMAC(EVP_sha256(),
                 secret_key.c_str(), secret_key.length(),
                 reinterpret_cast<const unsigned char *>(signature_data.c_str()),
                 signature_data.length(),
                 signature, &signature_len) == nullptr)
        {
            throw std::runtime_error("HMAC signature generation failed");
        }

        std::string signature_str(reinterpret_cast<char *>(signature), signature_len);
        std::string encoded_signature = url_safe_base64_encode(signature_str);

        return encoded_header + "." + encoded_payload + "." + encoded_signature;
    }
}

// URL安全的Base64编码函数
std::string url_safe_base64_encode(const std::string &data)
{
    static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    for (unsigned char c : data)
    {
        char_array_3[i++] = c;
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while (i++ < 3)
            ret += '=';
    }

    // 将 + 替换为 -, / 替换为 _
    for (char &c : ret)
    {
        if (c == '+')
            c = '-';
        else if (c == '/')
            c = '_';
    }

    return ret;
}

// URL安全的Base64解码函数
std::string url_safe_base64_decode(const std::string &encoded_string)
{
    std::string decoded_string = encoded_string;

    // 将URL安全字符转换回标准Base64字符
    std::replace(decoded_string.begin(), decoded_string.end(), '-', '+');
    std::replace(decoded_string.begin(), decoded_string.end(), '_', '/');

    // 添加必要的填充字符
    switch (decoded_string.length() % 4)
    {
    case 2:
        decoded_string += "==";
        break;
    case 3:
        decoded_string += "=";
        break;
    }

    // 标准Base64解码
    BIO *bio, *b64;
    int decode_length = calcDecodeLength(decoded_string);
    std::vector<unsigned char> buffer(decode_length);

    bio = BIO_new_mem_buf(decoded_string.c_str(), -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    int length = BIO_read(bio, buffer.data(), decoded_string.length());
    BIO_free_all(bio);

    return std::string(buffer.begin(), buffer.begin() + length);
}

// 计算Base64解码后的长度
int calcDecodeLength(const std::string &b64input)
{
    int len = b64input.length();
    int padding = 0;

    if (len >= 2)
    {
        if (b64input[len - 1] == '=')
            padding++;
        if (b64input[len - 2] == '=')
            padding++;
    }

    return (len * 3) / 4 - padding;
}

// 获取JWT密钥
std::string get_jwt_secret()
{
    const char *jwt_secret_env = std::getenv("JWT_SECRET");
    if (jwt_secret_env && std::strlen(jwt_secret_env) > 0)
    {
        return std::string(jwt_secret_env);
    }

    throw std::runtime_error("Missing required environment variable: JWT_SECRET");
}

// 生成JWT
std::string JwtUtils::createToken(int userId, const int type_id, const std::string &type_name, const std::string &identifier, bool isEmail)
{
    try
    {

        time_t now = time(nullptr);
        nlohmann::json payload_json;
        payload_json["sub"] = userId; // JWT 标准主体字段
        payload_json["type_id"] = type_id;
        payload_json["type_name"] = type_name;
        payload_json["identifier"] = identifier;
        payload_json["login_type"] = isEmail ? "email" : "phone";
        payload_json["iat"] = now; // 签发时间
        payload_json["exp"] = now + (RoleTypeUtils::isManagementRole(type_name)
                                         ? kManagementTokenTtlSeconds
                                         : kDefaultTokenTtlSeconds);

        return createSignedJwt(payload_json);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Token生成错误: " << e.what() << std::endl;
        throw; // 重新抛出异常
    }
}

// 创建修改安全信息操作凭证JWT
std::string JwtUtils::createUpdateTicket(int userId, const std::string &date, const std::string &identifier)
{
    time_t now = time(nullptr);
    nlohmann::json payload_json;
    payload_json["sub"] = userId;

    if (identifier == "email")
    {
        payload_json["email"] = date;
        payload_json["purpose"] = "change_email";
    }
    else if (identifier == "phone")
    {
        payload_json["phone"] = date;
        payload_json["purpose"] = "change_phone";
    }
    payload_json["iat"] = now;
    payload_json["exp"] = now + kEmailChangeTicketTtlSeconds;

    return createSignedJwt(payload_json);
}

// 解析并验证修改邮箱凭证JWT
std::optional<JwtUtils::EmailChangeTicketClaims> JwtUtils::getUpdateTicketClaims(const std::string &ticket, const std::string &date, const std::string &identifier)
{

    if (identifier != "email" && identifier != "phone")
    {
        return std::nullopt;
    }

    try
    {
        auto payload_opt = parseValidatedTokenPayload(ticket);
        if (!payload_opt)
        {
            return std::nullopt;
        }

        const auto &payload_json = payload_opt.value();

        // 检查凭证用途
        if (!payload_json.contains("purpose") ||
            !payload_json["purpose"].is_string() ||
            payload_json["purpose"].get<std::string>() != (identifier == "email" ? "change_email" : "change_phone"))
        {
            return std::nullopt;
        }

        // 检查用户ID和邮箱字段
        if (!payload_json.contains("sub") ||
            !payload_json["sub"].is_number_integer() ||
            !payload_json.contains(identifier == "email" ? "email" : "phone") ||
            !payload_json[identifier == "email" ? "email" : "phone"].is_string())
        {
            return std::nullopt;
        }

        return EmailChangeTicketClaims{
            payload_json["sub"].get<int>(),
            payload_json[identifier == "email" ? "email" : "phone"].get<std::string>()};
    }
    catch (const std::exception &e)
    {
        std::cerr << "Update ticket解析错误: " << e.what() << std::endl;
        return std::nullopt;
    }
}

// 验证JWT签名
bool verify_jwt_signature(const std::string &header, const std::string &payload,
                          const std::string &signature, const std::string &secret)
{
    std::string signature_data = header + "." + payload;

    unsigned char expected_signature[EVP_MAX_MD_SIZE];
    unsigned int signature_len;

    if (HMAC(EVP_sha256(), secret.c_str(), secret.length(),
             reinterpret_cast<const unsigned char *>(signature_data.c_str()),
             signature_data.length(), expected_signature, &signature_len) == nullptr)
    {
        return false;
    }

    std::string expected_signature_str(reinterpret_cast<char *>(expected_signature), signature_len);
    std::string actual_signature = url_safe_base64_decode(signature);

    return constantTimeEquals(expected_signature_str, actual_signature);
}

// 解析Token并提取Claims信息
std::optional<JwtUtils::TokenClaims> JwtUtils::getTokenClaims(const std::string &token)
{
    try
    {
        auto payload_opt = parseValidatedTokenPayload(token);
        if (!payload_opt)
        {
            return std::nullopt;
        }

        const auto &payload_json = payload_opt.value();
        if (!(payload_json.contains("sub") &&
              payload_json["sub"].is_number_integer()))
        {
            return std::nullopt;
        }

        const int userId = payload_json["sub"].get<int>();

        TokenClaims claims{
            userId,
            payload_json.contains("type_id") && payload_json["type_id"].is_number_integer() ? payload_json["type_id"].get<int>() : 0,
            payload_json.contains("type_name") && payload_json["type_name"].is_string() ? payload_json["type_name"].get<std::string>() : "",
            "",
            false};

        if (payload_json.contains("identifier") && payload_json["identifier"].is_string())
        {
            claims.identifier = payload_json["identifier"].get<std::string>();
        }
        else if (payload_json.contains("email") && payload_json["email"].is_string())
        {
            claims.identifier = payload_json["email"].get<std::string>();
            claims.isEmailLogin = true;
        }
        else if (payload_json.contains("phone") && payload_json["phone"].is_string())
        {
            claims.identifier = payload_json["phone"].get<std::string>();
            claims.isEmailLogin = false;
        }

        if (payload_json.contains("login_type") && payload_json["login_type"].is_string())
        {
            claims.isEmailLogin = payload_json["login_type"].get<std::string>() == "email";
        }

        return claims;
    }
    catch (const std::exception &e)
    {
        std::cerr << "JWT claims解析错误: " << e.what() << std::endl;
        return std::nullopt;
    }
}

// 获取用户存储在JWT中的用户ID
int JwtUtils::getUserIdFromToken(const std::string &token)
{
    try
    {
        auto claims = getTokenClaims(token);
        if (claims)
        {
            return claims->userId;
        }

        return -1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "JWT解析错误: " << e.what() << std::endl;
        return -1;
    }
}

// 验证用户对订单的访问权限
bool JwtUtils::isUserAuthorizedForOrder(int userId, int orderId, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    try
    {
        if (RoleTypeUtils::userHasBossRole(dbManager, userId))
        {
            return true;
        }

        // 需要执行SQL查询：
        // SELECT owner_id FROM orders WHERE id = orderId
        // 然后比较查询结果中的owner_id是否等于传入的userId

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT owner_id FROM orders WHERE id = ?")
                                       .bind(orderId)
                                       .execute();

        if (result.count() == 0)
        {
            return false; // 订单不存在
        }

        auto row = result.fetchOne();
        int orderOwnerId = row[0].get<int>();

        return orderOwnerId == userId; // 验证所有权
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in isUserAuthorizedForOrder: " << e.what() << std::endl;
        return false;
    }
}

// 验证用户对用户表单的访问权限
bool JwtUtils::isUserAuthorizedForUserForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    try
    {
        const auto target = getUserAuthTargetById(dbManager, userId);
        if (!target)
        {
            return false;
        }

        return target->userId == userId &&
               (RoleTypeUtils::isNormalUserRole(target->roleName) ||
                RoleTypeUtils::isBossRole(target->roleName));
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in isUserAuthorizedForUserForm: " << e.what() << std::endl;
        return false;
    }
}

// 验证管理员部门访问权限
bool JwtUtils::isUserAuthorizedForAdminForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    try
    {
        const auto target = getUserAuthTargetById(dbManager, userId);
        if (!target)
        {
            return false;
        }

        if (target->userId == userId && RoleTypeUtils::isManagementRole(target->roleName)) // 管理门户角色才能通过这个权限认证
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in isUserAuthorizedForAdminForm: " << e.what() << std::endl;
        return false;
    }
}

// 验证人事部门访问权限
bool JwtUtils::isUserAuthorizedForPersonnelForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    try
    {
        const auto target = getUserAuthTargetById(dbManager, userId);
        if (!target)
        {
            return false;
        }

        return target->userId == userId &&
               (RoleTypeUtils::isPersonnelRole(target->roleName) ||
                RoleTypeUtils::isBossRole(target->roleName));
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in isUserAuthorizedForPersonnelForm: " << e.what() << std::endl;
        return false;
    }
}

// 验证医疗端访问权限
bool JwtUtils::isUserAuthorizedForMedicalStaffForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    try
    {
        const auto target = getUserAuthTargetById(dbManager, userId);
        if (!target)
        {
            return false;
        }

        return target->userId == userId &&
               (RoleTypeUtils::isMedicalStaffRole(target->roleName) ||
                RoleTypeUtils::isBossRole(target->roleName));
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in isUserAuthorizedForMedicalStaffForm: " << e.what() << std::endl;
        return false;
    }
}

// 验证仓储端访问权限
bool JwtUtils::isUserAuthorizedForWarehouseStaffForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    try
    {
        const auto target = getUserAuthTargetById(dbManager, userId);
        if (!target)
        {
            return false;
        }

        return target->userId == userId &&
               (RoleTypeUtils::isWarehouseStaffRole(target->roleName) ||
                RoleTypeUtils::isBossRole(target->roleName));
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in isUserAuthorizedForWarehouseStaffForm: " << e.what() << std::endl;
        return false;
    }
}
