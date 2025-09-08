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

    void before_handle(crow::request &req, crow::response &res, context &ctx);
    void after_handle(crow::request &req, crow::response &res, context &ctx);
};

// 工具函数声明
void initializeCORS(const crow::request &req, crow::response &res);
void initializeOPTIONS(const crow::request &req, crow::response &res);
bool parseJsonBody(const crow::request &req, crow::response &res, nlohmann::json &request_body);

using CrowApp = crow::Crow<CorsMiddleware>;