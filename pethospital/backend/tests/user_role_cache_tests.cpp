#include "../services/redis/userRoleCache/UserRoleCache.h"

#include <cassert>

int main()
{
    // 键格式
    assert(UserRoleCache::userRoleKey(42) == "user:role:42");
    assert(UserRoleCache::userRoleTtlSeconds() == 300);

    // 无效 userId：读写失效都不触碰 Redis
    assert(!UserRoleCache::readCache(0).has_value());
    assert(!UserRoleCache::readCache(-1).has_value());
    assert(!UserRoleCache::writeCache(0, "医生"));

    // Redis 未启用（本测试不 init）：读穿透返回空、回填返回 false、失效 no-op
    assert(!UserRoleCache::readCache(42).has_value());
    assert(!UserRoleCache::writeCache(42, "医生"));

    // 空角色名永不缓存（避免把"查不到"固化）
    assert(!UserRoleCache::writeCache(42, ""));

    UserRoleCache::invalidate(42); // 不崩即可

    return 0;
}
