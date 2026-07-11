#pragma once

#include <string>

namespace AttendanceSecurity
{
std::string sha256Hex(const std::string &value);
std::string hmacSha256Hex(const std::string &message, const std::string &secret);
std::string buildCanonicalString(
    const std::string &method,
    const std::string &path,
    const std::string &timestamp,
    const std::string &nonce,
    const std::string &rawBody);
bool constantTimeEqualsHex(const std::string &left, const std::string &right);
bool verifySignature(
    const std::string &method,
    const std::string &path,
    const std::string &timestamp,
    const std::string &nonce,
    const std::string &rawBody,
    const std::string &secret,
    const std::string &providedSignature);
}
