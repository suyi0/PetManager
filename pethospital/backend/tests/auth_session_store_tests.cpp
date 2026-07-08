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
    // 会话版本现对全员生效（不再按角色名区分）；Redis 未启用时 issue 返回默认版本
    assert(AuthSessionStore::issueVersion(42) == AuthSessionStore::kDefaultSessionVersion);
    assert(AuthSessionStore::issueVersion(7) == AuthSessionStore::kDefaultSessionVersion);
    assert(AuthSessionStore::isSessionCurrent(7, 99)); // Redis 未启用 → 放行
    assert(!AuthSessionStore::isSessionCurrent(0, 0)); // 非法 userId → fail-closed
    assert(!RedisClient::instance().set("auth:test", "1"));

    // 第 6 参 managementSession 由调用方传入（只影响 TTL）；第 7 参 sessionVersion
    const std::string managementToken = JwtUtils::createToken(
        42, 6, "超级管理员", "admin@example.com", true, /*managementSession=*/true, 3);
    auto managementClaims = JwtUtils::getTokenClaims(managementToken);
    assert(managementClaims.has_value());
    assert(managementClaims->userId == 42);
    assert(managementClaims->sessionVersion == 3);

    const std::string userToken = JwtUtils::createToken(
        7, 1, "普通用户", "user@example.com", true, /*managementSession=*/false);
    auto userClaims = JwtUtils::getTokenClaims(userToken);
    assert(userClaims.has_value());
    assert(userClaims->userId == 7);
    assert(userClaims->sessionVersion == AuthSessionStore::kDefaultSessionVersion);

    unsetenv("JWT_SECRET");
    return 0;
}
