#include "AttendanceSecurity.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace
{
std::string bytesToHex(const unsigned char *data, unsigned int length)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < length; ++i)
    {
        stream << std::setw(2) << static_cast<int>(data[i]);
    }
    return stream.str();
}
}

namespace AttendanceSecurity
{
std::string sha256Hex(const std::string &value)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(value.data()), value.size(), digest);
    return bytesToHex(digest, SHA256_DIGEST_LENGTH);
}

std::string hmacSha256Hex(const std::string &message, const std::string &secret)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength = 0;
    unsigned char *result = HMAC(
        EVP_sha256(),
        secret.data(),
        static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char *>(message.data()),
        message.size(),
        digest,
        &digestLength);
    if (!result)
    {
        throw std::runtime_error("HMAC-SHA256 failed");
    }
    return bytesToHex(digest, digestLength);
}

std::string buildCanonicalString(
    const std::string &method,
    const std::string &path,
    const std::string &timestamp,
    const std::string &nonce,
    const std::string &rawBody)
{
    return method + "\n" + path + "\n" + timestamp + "\n" + nonce + "\n" + sha256Hex(rawBody);
}

bool constantTimeEqualsHex(const std::string &left, const std::string &right)
{
    if (left.size() != right.size())
    {
        return false;
    }
    unsigned char diff = 0;
    for (std::size_t i = 0; i < left.size(); ++i)
    {
        diff |= static_cast<unsigned char>(left[i] ^ right[i]);
    }
    return diff == 0;
}

bool verifySignature(
    const std::string &method,
    const std::string &path,
    const std::string &timestamp,
    const std::string &nonce,
    const std::string &rawBody,
    const std::string &secret,
    const std::string &providedSignature)
{
    if (secret.empty() || providedSignature.empty())
    {
        return false;
    }
    const std::string canonical = buildCanonicalString(method, path, timestamp, nonce, rawBody);
    const std::string expected = hmacSha256Hex(canonical, secret);
    return constantTimeEqualsHex(expected, providedSignature);
}
}
