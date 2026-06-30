#pragma once

#include <optional>
#include <string>

// 验证码的 Redis 存取（key = `verify:code:<identifier>`，原生 TTL，多实例共享）。
// 只封 Redis 的 key / TTL / 一次性消费；验证码生成、SMTP/SMS 发送、本地内存兜底、
// 常量时间比较仍留在 `Verify`。Redis 未启用或出错时：store→false、get→nullopt、del→false，
// 调用方据此回退本地内存（identifier 实际承载邮箱或手机号）。
namespace VerificationCodeRedisStore
{
    bool store(const std::string &identifier, const std::string &code, int ttlSeconds);
    std::optional<std::string> get(const std::string &identifier);
    bool del(const std::string &identifier);
}
