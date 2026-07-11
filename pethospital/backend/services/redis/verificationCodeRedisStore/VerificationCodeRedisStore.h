#pragma once

#include <optional>
#include <string>

// 验证码的 Redis 存取（key = `verify:code:<identifier>`，原生 TTL，多实例共享）。
// 只封 Redis 的 key / TTL / 一次性消费；验证码生成、SMTP/SMS 发送、本地内存兜底、
// 常量时间比较仍留在 `Verify`。Redis 未启用或出错时：store→false、get→nullopt、del→false，
// 调用方据此回退本地内存（identifier 实际承载邮箱或手机号）。
namespace VerificationCodeRedisStore
{
    // 兼容判断 Redis 是否启用的函数 和 Redis 的存储验证码函数，供 Verify 调用。
    bool store(const std::string &identifier, const std::string &code, int ttlSeconds);

    // 兼容判断 Redis 是否启用的函数 和 Redis 的获取验证码函数，供 Verify 调用。
    std::optional<std::string> get(const std::string &identifier);

    // 兼容判断 Redis 是否启用的函数 和 Redis 的删除验证码函数，供 Verify 调用。
    bool del(const std::string &identifier);
}
