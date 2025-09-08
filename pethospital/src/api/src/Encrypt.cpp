#include "../include/Encrypt.h"

// 添加辅助函数
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

std::string read_config_value(const std::string &filename, const std::string &key)
{
    std::ifstream config_file(filename);
    std::string line;

    while (std::getline(config_file, line))
    {
        // 跳过注释行和空行
        if (line.empty() || line[0] == '#' || line[0] == ';')
        {
            continue;
        }

        // 查找键值对 (格式: KEY=VALUE)
        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos)
        {
            std::string config_key = line.substr(0, delimiter_pos);
            // 去除空格
            config_key.erase(0, config_key.find_first_not_of(" \t"));
            config_key.erase(config_key.find_last_not_of(" \t") + 1);

            if (config_key == key)
            {
                std::string value = line.substr(delimiter_pos + 1);
                // 去除空格和引号
                value.erase(0, value.find_first_not_of(" \t\""));
                value.erase(value.find_last_not_of(" \t\"") + 1);
                return value;
            }
        }
    }

    return ""; // 未找到键值
}
std::string get_jwt_secret()
{
    // 首先尝试从环境变量获取
    const char *jwt_secret_env = std::getenv("JWT_SECRET");
    if (jwt_secret_env)
    {
        // 如果环境变量存在，则返回该值
        std::cout << "Using JWT_SECRET from environment variable." << std::endl;
        return std::string(jwt_secret_env);
    }

    // 如果环境变量不存在，则从配置文件读取
    std::string secret_from_file = read_config_value("config.ini", "JWT_SECRET");
    if (!secret_from_file.empty())
    {
        // 如果配置文件存在，则返回该值
        std::cout << "Using JWT_SECRET from config.ini." << std::endl;
        return secret_from_file;
    }

    // 如果都没有，则使用默认值（仅用于开发环境）
    return "default-secret-key-change-in-production";
}
std::string generate_SHA256_jwt(const std::string &email)
{
    // JWT由三部分组成: header.payload.signature

    // 1. Header
    std::string header = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    std::string encoded_header = url_safe_base64_encode(header);

    // 2. Payload
    time_t now = time(nullptr);
    nlohmann::json payload_json;
    payload_json["email"] = email;
    payload_json["iat"] = now;
    payload_json["exp"] = now + 36000; // 10小时过期

    std::string payload = payload_json.dump();
    std::string encoded_payload = url_safe_base64_encode(payload);

    // 3. Signature (使用HMAC-SHA256算法)
    std::string signature_data = encoded_header + "." + encoded_payload;

    // 获取JWT密钥
    std::string secret_key = get_jwt_secret();

    // 使用HMAC-SHA256生成签名
    unsigned char *signature = HMAC(EVP_sha256(),
                                    secret_key.c_str(),
                                    secret_key.length(),
                                    reinterpret_cast<const unsigned char *>(signature_data.c_str()),
                                    signature_data.length(),
                                    nullptr,
                                    nullptr);

    // 对签名进行URL安全的Base64编码
    std::string signature_str(reinterpret_cast<char *>(signature), 32); // SHA256 produces 32 bytes
    std::string encoded_signature = url_safe_base64_encode(signature_str);

    return encoded_header + "." + encoded_payload + "." + encoded_signature;
}

std::string sha256_hash(const std::string &input)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // 使用新的EVP API替代已弃用的SHA256_*函数
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr)
    {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to initialize SHA256 digest");
    }

    if (EVP_DigestUpdate(mdctx, input.c_str(), input.size()) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to update SHA256 digest");
    }

    unsigned int hash_len;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to finalize SHA256 digest");
    }

    EVP_MD_CTX_free(mdctx);

    // 将哈希值转换为十六进制字符串
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}
