#include "../controllers/auth/jwtUtils/jwtUtils.h"
#include "../services/auth/AuthSessionStore.h"
#include "../services/redis/RedisClient.h"

#include <cassert>
#include <cstdlib>
#include <optional>

int main()
{
    setenv("JWT_SECRET", "test-secret-for-auth-session-store", 1);

    assert(AuthSessionStore::sessionVersionKey(42) == "auth:session-version:42");
    assert(AuthSessionStore::isTokenVersionCurrent(0, std::nullopt));
    assert(AuthSessionStore::isTokenVersionCurrent(2, std::optional<int>{2}));
    assert(!AuthSessionStore::isTokenVersionCurrent(1, std::optional<int>{2}));
    assert(AuthSessionStore::issueVersionForRole(42, "超级管理员") == AuthSessionStore::kDefaultSessionVersion);
    assert(AuthSessionStore::issueVersionForRole(7, "普通用户") == AuthSessionStore::kDefaultSessionVersion);
    assert(AuthSessionStore::isSessionCurrent(7, "普通用户", 99));
    assert(!AuthSessionStore::isSessionCurrent(0, "超级管理员", 0));
    assert(!RedisClient::instance().set("auth:test", "1"));

    const std::string managementToken = JwtUtils::createToken(
        42, 6, "超级管理员", "admin@example.com", true, 3);
    auto managementClaims = JwtUtils::getTokenClaims(managementToken);
    assert(managementClaims.has_value());
    assert(managementClaims->userId == 42);
    assert(managementClaims->sessionVersion == 3);

    const std::string userToken = JwtUtils::createToken(
        7, 1, "普通用户", "user@example.com", true);
    auto userClaims = JwtUtils::getTokenClaims(userToken);
    assert(userClaims.has_value());
    assert(userClaims->userId == 7);
    assert(userClaims->sessionVersion == AuthSessionStore::kDefaultSessionVersion);

    unsetenv("JWT_SECRET");
    return 0;
}
