#pragma once
#include <crow.h>
#include <nlohmann/json.hpp>

// CORS中间件类
class CorsMiddleware
{
public:
    struct context
    {
    };

    // 使用默认构造函数
    CorsMiddleware() = default;

    void before_handle(crow::request &req, crow::response &res, context &ctx);
    void after_handle(crow::request &req, crow::response &res, context &ctx);
};