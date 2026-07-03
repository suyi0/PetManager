#pragma once

#include <crow.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <string>

// 请求相关的公共小工具。header-only（inline），避免新增 .cpp 牵动各 CMake 目标的链接。
namespace RequestUtils
{
    // 从 JSON 请求体取字符串字段：字段存在且为字符串才返回，否则返回 fallback。
    // 注意与全局 getRequestString 的差异：后者对非字符串值会 .dump() 成串，这里直接回退。
    // 原先在 finance/admin/warehouseManager 三个 handler 各写一份，现统一到此处。
    inline std::string getJsonString(const nlohmann::json &body, const std::string &key, const std::string &fallback = "")
    {
        return body.contains(key) && body[key].is_string() ? body[key].get<std::string>() : fallback;
    }

    // 从 JSON 请求体取整数字段：字段存在且为整数才返回其值，否则返回 fallback。
    // 原先在 finance/admin/warehouseManager 三个 handler 各写一份，现统一到此处。
    inline int getJsonInt(const nlohmann::json &body, const std::string &key, int fallback)
    {
        return body.contains(key) && body[key].is_number_integer() ? body[key].get<int>() : fallback;
    }

    // 规整页码：至少为 1。
    inline int normalizePage(int page)
    {
        return std::max(1, page);
    }

    // 规整每页条数：<=0 用 fallback，且不超过 max（防止单页拉取过多拖垮 DB）。
    inline int normalizePageSize(int pageSize, int fallback = 10, int max = 100)
    {
        if (pageSize <= 0)
        {
            return fallback;
        }
        return std::min(pageSize, max);
    }

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
