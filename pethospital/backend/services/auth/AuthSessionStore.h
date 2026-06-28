#pragma once

#include <optional>
#include <string>

namespace AuthSessionStore
{
    constexpr int kDefaultSessionVersion = 0;

    std::string sessionVersionKey(int userId);
    std::optional<int> parseSessionVersion(const std::optional<std::string> &raw);
    bool isTokenVersionCurrent(int tokenVersion, const std::optional<int> &storedVersion);
    int issueVersionForRole(int userId, const std::string &roleName);
    bool isSessionCurrent(int userId, const std::string &roleName, int tokenVersion);
    bool bumpSessionVersionForUser(int userId);
}
