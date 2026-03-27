#include "JwtUtils.h"
#include <cstring>

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
std::string JwtUtils::createToken(int userId, const std::string &username, const int type_id, const std::string &type_name, const std::string &identifier, bool isEmail)
{
    try
    {
        // JWT由三部分组成: header.payload.signature
    
        // 1. Header
        std::string header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
        std::string encoded_header = url_safe_base64_encode(header);
    
        // 2. Payload
        time_t now = time(nullptr);
        nlohmann::json payload_json;
        payload_json["id"] = userId;
        payload_json["username"] = username;
        payload_json["type_id"] = type_id;
        payload_json["type_name"] = type_name;
        payload_json["identifier"] = identifier;

        if(isEmail) {
            payload_json["login_type"] = "email";
            payload_json["email"] = identifier;
        } else {
            payload_json["login_type"] = "phone";
            payload_json["phone"] = identifier;
        }
        payload_json["iat"] = now;           // 签发时间
        if(type_name == "超级管理员")
        {
            payload_json["exp"] = now + 300; // 超级管理员的JWT五分钟后过期
        }
        else
        {
            payload_json["exp"] = now + 604800;  // 一周有效
        }
    
        std::string payload = payload_json.dump();
        std::string encoded_payload = url_safe_base64_encode(payload); // 对负载进行URL安全的Base64编码
    
        // 3. Signature (使用HMAC-SHA256算法)
        std::string signature_data = encoded_header + "." + encoded_payload;
    
        // 获取JWT密钥
        std::string secret_key = get_jwt_secret();
    
        // 使用HMAC-SHA256生成签名
        unsigned char signature[EVP_MAX_MD_SIZE]; // 定义签名长度为EVP_MAX_MD_SIZE = 64 的缓冲区
        unsigned int signature_len;
    
        // HMAC(EVP_sha256(), key, key_len, data, data_len, md, md_len)
        // key - 密钥, key_len - 密钥长度
        // data - 待签名的数据, data_len - 待签名的数据长度
        // md - 签名结果, md_len - 签名结果长度
        if (HMAC(EVP_sha256(),
                 secret_key.c_str(), secret_key.length(),
                 reinterpret_cast<const unsigned char *>(signature_data.c_str()),
                 signature_data.length(),
                 signature, &signature_len) == nullptr)
        {
            throw std::runtime_error("HMAC signature generation failed");
        }
    
        // 对签名进行URL安全的Base64编码
        std::string signature_str(reinterpret_cast<char *>(signature), 32); // SHA256 produces 32 bytes
        std::string encoded_signature = url_safe_base64_encode(signature_str);
    
        return encoded_header + "." + encoded_payload + "." + encoded_signature;
    } catch (const std::exception& e) {
        std::cerr << "Token生成错误: " << e.what() << std::endl;
        throw; // 重新抛出异常
    }
}

// 验证JWT签名
bool verify_jwt_signature(const std::string &header, const std::string &payload,
                          const std::string &signature, const std::string &secret)
{
    std::string signature_data = header + "." + payload;

    unsigned char expected_signature[EVP_MAX_MD_SIZE];
    unsigned int signature_len;

    HMAC(EVP_sha256(), secret.c_str(), secret.length(),
         reinterpret_cast<const unsigned char *>(signature_data.c_str()),
         signature_data.length(), expected_signature, &signature_len);

    std::string expected_signature_str(reinterpret_cast<char *>(expected_signature), signature_len);
    std::string actual_signature = url_safe_base64_decode(signature);

    return expected_signature_str == actual_signature;
}

// 解析JWT token获取用户ID
int JwtUtils::getUserIdFromToken(const std::string &token)
{
    try
    {
        // 分割JWT的三个部分
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);

        if (first_dot == std::string::npos || second_dot == std::string::npos)
        {
            return -1; // 无效的JWT格式
        }

        std::string header_encoded = token.substr(0, first_dot);
        std::string payload_encoded = token.substr(first_dot + 1, second_dot - first_dot - 1);
        std::string signature_encoded = token.substr(second_dot + 1);

        // 验证签名
        std::string secret_key = get_jwt_secret();
        if (!verify_jwt_signature(header_encoded, payload_encoded, signature_encoded, secret_key))
        {
            return -2; // 签名验证失败
        }

        // 解码payload
        std::string payload_decoded = url_safe_base64_decode(payload_encoded);
        nlohmann::json payload_json = nlohmann::json::parse(payload_decoded);

        // 检查过期时间
        time_t now = time(nullptr);
        if (payload_json.contains("exp") && payload_json["exp"].get<time_t>() < now)
        {
            return -3; // token已过期
        }

        if (payload_json.contains("id") && payload_json["id"].is_number_integer())
        {
            return payload_json["id"].get<int>();
        }

        return -4; // 用户不存在
    }
    catch (const std::exception &e)
    {
        std::cerr << "JWT解析错误: " << e.what() << std::endl;
        return -5; // 解析异常
    }
}

// 获取用户
std::string JwtUtils::getUserIdentifierFromToken(const std::string &token)
{
    try
    {
        // 分割JWT的三个部分
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);

        if (first_dot == std::string::npos || second_dot == std::string::npos)
        {
            return ""; // 无效的JWT格式
        }

        std::string header_encoded = token.substr(0, first_dot);
        std::string payload_encoded = token.substr(first_dot + 1, second_dot - first_dot - 1);
        std::string signature_encoded = token.substr(second_dot + 1);

        // 验证签名
        std::string secret_key = get_jwt_secret();
        if (!verify_jwt_signature(header_encoded, payload_encoded, signature_encoded, secret_key))
        {
            return ""; // 签名验证失败
        }

        // 解码payload
        std::string payload_decoded = url_safe_base64_decode(payload_encoded);
        nlohmann::json payload_json = nlohmann::json::parse(payload_decoded);

        // 检查过期时间
        time_t now = time(nullptr);
        if (payload_json.contains("exp") && payload_json["exp"].get<time_t>() < now)
        {
            return ""; // token已过期
        }

        // 直接从payload中获取identifier字段
        if (payload_json.contains("identifier"))
        {
            return payload_json["identifier"].get<std::string>();
        }

        // 如果没有identifier字段，尝试从email或phone字段获取
        if (payload_json.contains("email"))
        {
            return payload_json["email"].get<std::string>();
        }
        else if (payload_json.contains("phone"))
        {
            return payload_json["phone"].get<std::string>();
        }

        return ""; // 用户不存在
    }
    catch (const std::exception &e)
    {
        std::cerr << "JWT解析错误: " << e.what() << std::endl;
        return ""; // 解析异常
    }
}

// 验证用户对订单的访问权限
bool JwtUtils::isUserAuthorizedForOrder(int userId, int orderId, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    try
    {
        // 需要执行SQL查询：
        // SELECT user_id FROM orders WHERE id = orderId
        // 然后比较查询结果中的user_id是否等于传入的userId
    
        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT user_id FROM orders WHERE id = ?")
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
    catch (const std::exception& e) {
        std::cerr << "Error in isUserAuthorizedForOrder: " << e.what() << std::endl;
        return false;
    }
    
}

// 验证用户对用户表单的访问权限
bool JwtUtils::isUserAuthorizedForUserForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    try
    {
        // 需要执行SQL查询：
        // SELECT user_id From users WHERE id = orderId
    
        mysqlx::RowResult result;
        if(isEmail)
        {
            result = dbManager->getSession()
                        ->sql("SELECT u.id, t.type FROM users AS u "
                              "JOIN types AS t ON u.type_id = t.id "
                              "WHERE u.email = ?")
                        .bind(identifier)
                        .execute();
        }
        else
        {
            result = dbManager->getSession()
                        ->sql("SELECT u.id, t.type FROM users AS u "
                              "JOIN types AS t ON u.type_id = t.id "
                              "WHERE u.phone = ?")
                        .bind(identifier)
                        .execute();
        }
    
        // 检查是否找到用户
        if(result.count() == 0)
        {
            return false;
        }
    
        auto row = result.fetchOne();
        int userOwnerId = row[0].get<int>();
        std::string userRoleName = row[1].get<std::string>();
        if(userRoleName == "超级管理员")                 // 超级管理员允许大部份的操作
        {
            return true;
        }
    
        return userOwnerId == userId;       // 普通用户和医生只能操作自己
    }
    catch (const std::exception& e) {
        std::cerr << "Error in isUserAuthorizedForUserForm: " << e.what() << std::endl;
        return false;
    }
}

// 验证管理员表单的访问权限
bool JwtUtils::isUserAuthorizedForAdminForm(int userId, std::string &identifier, bool isEmail, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    try
    {

        mysqlx::RowResult result;
    
        if(isEmail)
        {
            result = dbManager->getSession()
                        ->sql("SELECT u.id, t.type FROM users AS u "
                              "JOIN types AS t ON u.type_id = t.id "
                              "WHERE u.email = ?")
                        .bind(identifier)
                        .execute();
        }
        else
        {
            result = dbManager->getSession()
                        ->sql("SELECT u.id, t.type FROM users AS u "
                              "JOIN types AS t ON u.type_id = t.id "
                              "WHERE u.phone = ?")
                        .bind(identifier)
                        .execute();
        }
    
        // 检查是否找到用户
        if(result.count() == 0)
        {
            return false;
        }
    
        auto row = result.fetchOne();
        int foundUserId = row[0].get<int>();
        std::string userRoleName = row[1].get<std::string>();
    
        if(foundUserId == userId && userRoleName == "超级管理员")                 // 只有超级管理员才能通过这个权限认证
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error in isUserAuthorizedForAdminForm: " << e.what() << std::endl;
        return false;
    }
}
