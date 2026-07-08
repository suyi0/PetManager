#pragma once

#include <optional>
#include <string>

// 每用户角色名缓存（userId -> 角色中文名）。
//
// 为什么用共享 Redis 而非进程内缓存、也不用 Pub/Sub：
//   缓存值直接存在共享 Redis 里，角色变更时任一实例 DEL 掉该用户的 key，
//   对所有实例立即生效——这正是共享缓存相对"每实例各持本地副本"（WS 广播器那种）
//   的优势，无需再发订阅消息。失效用"每用户 DEL"而非全局版本号，因为一个用户改角色
//   不该让所有人缓存失效。
//
// 为什么值得缓存：getUserRoleName 在 order/reservation/search 公共权限校验里
//   每个已登录请求都要 `users JOIN positions` 查一次库，是真正的热路径。
//
// 降级：Redis 未启用时读写都 no-op，getUserRoleName 直接回退到 DB 查询（无缓存亦无陈旧）。
namespace UserRoleCache
{
    // 键：user:role:<userId>
    std::string userRoleKey(int userId);

    // 安全网 TTL：DEL 才是真正的即时失效机制，TTL 只兜底"漏掉/失败的一次 DEL"
    // 造成的陈旧上限，并回收孤儿键。5 分钟与既有 medicine 缓存约定一致。
    int userRoleTtlSeconds();

    // 读穿透：命中返回缓存的角色名；未命中 / Redis 未启用返回 nullopt。
    std::optional<std::string> readCache(int userId);

    // 回填：仅缓存非空角色名（SETEX）。空名不缓存，避免把"查不到"固化下来。
    bool writeCache(int userId, const std::string &roleName);

    // 失效：删除该用户的角色缓存（DEL）。角色变更（改派/取消权限）后调用。
    void invalidate(int userId);
}
