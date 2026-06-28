#pragma once

#include <crow.h>

#include <array>
#include <string>

// 请求相关的公共小工具。header-only（inline），避免新增 .cpp 牵动各 CMake 目标的链接。
namespace RequestUtils
{
    // 按常见代理头优先级取真实客户端 IP（多值时取第一个），都没有则 "unknown"。
    // 全局唯一来源：日志 / 限流 / 登录锁定都用它，避免各写一份漂移。
    inline std::string getClientIp(const crow::request &req)
    {
        static const std::array<const char *, 5> headers = {
            "X-Forwarded-For",
            "X-Real-IP",
            "CF-Connecting-IP",
            "X-Original-For",
            "X-Cluster-Client-IP"};

        for (const char *header : headers)
        {
            std::string value = req.get_header_value(header);
            if (!value.empty())
            {
                const size_t pos = value.find(',');
                return pos == std::string::npos ? value : value.substr(0, pos);
            }
        }

        return "unknown";
    }
}
