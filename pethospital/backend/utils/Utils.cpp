#include "Utils.h"

// 解析请求体中的 JSON 数据
bool parseJsonBody(const crow::request &req, crow::response &res, nlohmann::json &request_body)
{
    try
    {
        request_body = nlohmann::json::parse(req.body);
    }
    catch (...)
    {
        res.code = 400;
        res.set_header("Content-Type", "application/json");
        std::string origin = req.get_header_value("Origin");
        if (origin == "http://localhost:8080" || origin == "http://127.0.0.1:8080")
        {
            res.set_header("Access-Control-Allow-Origin", origin);
        }
        else
        {
            res.set_header("Access-Control-Allow-Origin", origin.empty() ? "*" : origin);
        }
        res.write(R"({"error": "Invalid JSON"})");
        res.end(); // 显式结束响应
        return false;
    }
    return true;
}

// 获得创建时间
std::string getCreateTime()
{
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now = *std::localtime(&time_now);
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    std::string creation_time = oss.str();

    return creation_time;
}

// 判断响应结果函数
void ProcessHandlerResponse(const crow::request &req, crow::response &res, crow::response &handlerResponse)
{
    nlohmann::json response_json;
    if (!handlerResponse.body.empty())
    {
        try
        {
            response_json = nlohmann::json::parse(handlerResponse.body);
        }
        catch (const std::exception &e)
        {
            // 如果解析失败，使用原始响应
            res.code = handlerResponse.code;
            res.body = handlerResponse.body;
            return;
        }
    }
    res = ResponseHelper::custom(req, handlerResponse.code, response_json);
}
