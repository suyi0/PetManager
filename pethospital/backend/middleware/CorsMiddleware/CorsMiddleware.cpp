#include "corsMiddleware.h"

// 设置CORS头中间件
void CorsMiddleware::before_handle(crow::request &req, crow::response &res, context &ctx)
{
    // 设置CORS头
    // 动态设置允许的源
    std::string origin = req.get_header_value("Origin");
    // 检查origin是否在允许的列表中
    if (origin == "http://localhost:8080" || origin == "http://127.0.0.1:8080")
    {
        res.set_header("Access-Control-Allow-Origin", origin.empty() ? "*" : origin);
    }
    else
    {
        // 默认只允许localhost:8080
        res.set_header("Access-Control-Allow-Origin", "http://localhost:8080");
    }

    // 其他CORS头
    // 允许的 HTTP 方法
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
    // 允许的请求头
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    res.set_header("Access-Control-Max-Age", "86400");
    res.set_header("Access-Control-Allow-Credentials", "true");

    // 对于OPTIONS预检请求，直接返回204
    if (req.method == crow::HTTPMethod::Options)
    {
        res.code = 204;
        res.end();
    }
}

void CorsMiddleware::after_handle(crow::request &req, crow::response &res, context &ctx)
{
    // 确保所有响应都包含CORS头
    std::string origin = req.get_header_value("Origin");
    if (origin == "http://localhost:8080" || origin == "http://127.0.0.1:8080")
    {
        res.set_header("Access-Control-Allow-Origin", origin);
    }
    else
    {
        res.set_header("Access-Control-Allow-Origin", origin.empty() ? "*" : origin);
    }
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    res.set_header("Access-Control-Allow-Credentials", "true");
}
