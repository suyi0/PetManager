#pragma once

#include <crow.h>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

// CORS 相关的公共小工具。header-only（inline），避免新增 .cpp 牵动各 CMake 目标的链接。
// 目的：把原先在 corsMiddleware(before/after) 与 rateLimitMiddleware 三处各写一份、
// 且行为已漂移（after_handle 会回显任意 Origin，配合 credentials=true 是个弱点）的
// CORS 头逻辑收敛到一处，并让允许来源可通过环境变量在部署时覆盖。
namespace CorsUtils
{
    // 允许的前端来源：环境变量 CORS_ALLOWED_ORIGINS（逗号分隔）；默认开发环境的
    // localhost:8080 / 127.0.0.1:8080。只解析一次并缓存。
    inline const std::vector<std::string> &allowedOrigins()
    {
        static const std::vector<std::string> origins = []
        {
            std::vector<std::string> result;
            const char *raw = std::getenv("CORS_ALLOWED_ORIGINS");
            const std::string value = raw ? raw : "http://localhost:8080,http://127.0.0.1:8080";

            size_t start = 0;
            while (start <= value.size())
            {
                const size_t comma = value.find(',', start);
                const size_t len = (comma == std::string::npos) ? std::string::npos : comma - start;
                std::string token = value.substr(start, len);

                const size_t b = token.find_first_not_of(" \t");
                const size_t e = token.find_last_not_of(" \t");
                if (b != std::string::npos)
                {
                    result.push_back(token.substr(b, e - b + 1));
                }

                if (comma == std::string::npos)
                {
                    break;
                }
                start = comma + 1;
            }

            if (result.empty())
            {
                result.push_back("http://localhost:8080");
            }
            return result;
        }();
        return origins;
    }

    // Origin 是否在允许列表内。
    inline bool isAllowed(const std::string &origin)
    {
        const auto &list = allowedOrigins();
        return std::find(list.begin(), list.end(), origin) != list.end();
    }

    // 决定 Access-Control-Allow-Origin 的取值：命中允许列表则回显该来源，
    // 否则回退到列表首项——不再回显任意来源（修正原 after_handle 的 "*"/任意 origin 弱点）。
    inline std::string resolveAllowOrigin(const std::string &origin)
    {
        return isAllowed(origin) ? origin : allowedOrigins().front();
    }

    // 统一写入一组 CORS 响应头（来源/方法/头/凭证）。不含 Max-Age——预检的 Max-Age
    // 只在 before_handle 里额外设置，保持原行为。
    inline void applyCorsHeaders(crow::response &res, const std::string &origin)
    {
        res.set_header("Access-Control-Allow-Origin", resolveAllowOrigin(origin));
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
        res.set_header("Access-Control-Allow-Credentials", "true");
    }
}
