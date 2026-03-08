#include "Encrypt.h"

// SHA256哈希函数
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
