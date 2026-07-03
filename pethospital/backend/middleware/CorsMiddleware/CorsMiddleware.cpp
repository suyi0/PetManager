#include "corsMiddleware.h"
#include "../../utils/corsUtils/CorsUtils.h"

// 设置CORS头中间件
void CorsMiddleware::before_handle(crow::request &req, crow::response &res, context &ctx)
{
    // 来源/方法/头/凭证统一走 CorsUtils（允许列表可用 CORS_ALLOWED_ORIGINS 覆盖）。
    const std::string origin = req.get_header_value("Origin");
    CorsUtils::applyCorsHeaders(res, origin);
    res.set_header("Access-Control-Max-Age", "86400");

    // 对于OPTIONS预检请求，直接返回204
    if (req.method == crow::HTTPMethod::Options)
    {
        res.code = 204;
        res.end();
    }
}

void CorsMiddleware::after_handle(crow::request &req, crow::response &res, context &ctx)
{
    // 确保所有响应都包含CORS头（与 before_handle 用同一份允许列表，不再回显任意来源）。
    const std::string origin = req.get_header_value("Origin");
    CorsUtils::applyCorsHeaders(res, origin);
}
