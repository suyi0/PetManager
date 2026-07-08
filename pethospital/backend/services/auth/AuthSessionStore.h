#pragma once

#include <optional>
#include <string>

namespace AuthSessionStore
{
    constexpr int kDefaultSessionVersion = 0;

    std::string sessionVersionKey(int userId);
    std::optional<int> parseSessionVersion(const std::optional<std::string> &raw);
    bool isTokenVersionCurrent(int tokenVersion, const std::optional<int> &storedVersion);
    // 会话版本全员生效（动态 RBAC：派职位/改权限的 bump 必须对任何账户起作用，不按 token 里的旧角色名决定）
    int issueVersion(int userId);
    bool isSessionCurrent(int userId, int tokenVersion);
    bool bumpSessionVersionForUser(int userId);
}
