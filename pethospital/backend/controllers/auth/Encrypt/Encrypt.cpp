#include "encrypt.h"

namespace
{
constexpr int kPbkdf2Iterations = 150000;
constexpr size_t kSaltSize = 16;
constexpr size_t kDerivedKeySize = 32;
const std::string kPasswordScheme = "pbkdf2_sha256";

std::string bytes_to_hex(const unsigned char *data, size_t len)
{
    std::stringstream ss;
    for (size_t i = 0; i < len; ++i)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::string bytes_to_hex(const std::vector<unsigned char> &data)
{
    return bytes_to_hex(data.data(), data.size());
}

std::vector<unsigned char> hex_to_bytes(const std::string &hex)
{
    if (hex.size() % 2 != 0)
    {
        throw std::runtime_error("Invalid hex length");
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2)
    {
        bytes.push_back(static_cast<unsigned char>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    }

    return bytes;
}

std::vector<std::string> split_string(const std::string &value, char delimiter)
{
    std::vector<std::string> parts;
    std::stringstream ss(value);
    std::string item;
    while (std::getline(ss, item, delimiter))
    {
        parts.push_back(item);
    }
    return parts;
}

bool is_legacy_sha256_hash(const std::string &stored_hash)
{
    if (stored_hash.size() != SHA256_DIGEST_LENGTH * 2)
    {
        return false;
    }

    return std::all_of(stored_hash.begin(), stored_hash.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

std::vector<unsigned char> derive_pbkdf2_hash(const std::string &password,
                                              const std::vector<unsigned char> &salt,
                                              int iterations)
{
    std::vector<unsigned char> derived_key(kDerivedKeySize);
    if (PKCS5_PBKDF2_HMAC(password.c_str(),
                          static_cast<int>(password.size()),
                          salt.data(),
                          static_cast<int>(salt.size()),
                          iterations,
                          EVP_sha256(),
                          static_cast<int>(derived_key.size()),
                          derived_key.data()) != 1)
    {
        throw std::runtime_error("Failed to derive PBKDF2 password hash");
    }

    return derived_key;
}
} // namespace

// 计算SHA256哈希值
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

// 生成带盐的密码哈希
std::string hash_password(const std::string &password)
{
    std::vector<unsigned char> salt(kSaltSize);
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1)
    {
        throw std::runtime_error("Failed to generate password salt");
    }

    std::vector<unsigned char> derived_key = derive_pbkdf2_hash(password, salt, kPbkdf2Iterations);

    return kPasswordScheme + "$" + std::to_string(kPbkdf2Iterations) + "$" +
           bytes_to_hex(salt) + "$" + bytes_to_hex(derived_key);
}
// 校验密码哈希(一致：true, 不一致：false)
bool verify_password_hash(const std::string &password, const std::string &stored_hash)
{
    if (stored_hash.rfind(kPasswordScheme + "$", 0) == 0)
    {
        std::vector<std::string> parts = split_string(stored_hash, '$');
        if (parts.size() != 4)
        {
            return false;
        }

        const int iterations = std::stoi(parts[1]);
        const std::vector<unsigned char> salt = hex_to_bytes(parts[2]);
        const std::vector<unsigned char> expected_hash = hex_to_bytes(parts[3]);
        const std::vector<unsigned char> actual_hash = derive_pbkdf2_hash(password, salt, iterations);

        if (expected_hash.size() != actual_hash.size())
        {
            return false;
        }

        return CRYPTO_memcmp(expected_hash.data(), actual_hash.data(), expected_hash.size()) == 0;
    }

    if (is_legacy_sha256_hash(stored_hash))
    {
        return sha256_hash(password) == stored_hash;
    }

    return false;
}

// 判断是否需要升级旧哈希
bool password_hash_needs_upgrade(const std::string &stored_hash)
{
    if (is_legacy_sha256_hash(stored_hash))
    {
        return true;
    }

    if (stored_hash.rfind(kPasswordScheme + "$", 0) != 0)
    {
        return true;
    }

    std::vector<std::string> parts = split_string(stored_hash, '$');
    if (parts.size() != 4)
    {
        return true;
    }

    return std::stoi(parts[1]) < kPbkdf2Iterations;
}
