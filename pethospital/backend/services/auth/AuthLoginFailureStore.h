#pragma once

#include <string>

// 登录失败锁定（多实例共享，Redis 实现）。按「客户端 IP + 标识符」组合维度计数与锁定，
// 避免攻击者拿受害者标识符把对方账号锁死的 DoS；对不存在的标识符同样计数，避免账号枚举。
// 阈值与时长（5 次 / 15 分钟）封装在内。Redis 未启用：isLocked→false、记录/清理→no-op（不阻断登录）。
namespace AuthLoginFailureStore
{
    // 该 IP+标识符当前是否被锁定。
    bool isLocked(const std::string &clientIp, const std::string &identifier);
    // 记一次登录失败：INCR 计数，首次开窗 TTL，达阈值则上锁。
    void recordFailure(const std::string &clientIp, const std::string &identifier);
    // 登录成功：清掉失败计数与锁。
    void clearOnSuccess(const std::string &clientIp, const std::string &identifier);
}
