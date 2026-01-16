#pragma once
#include <crow.h>
#include <nlohmann/json.hpp>


// 工具函数声明
void initializeCORS(const crow::request &req, crow::response &res);
void initializeOPTIONS(const crow::request &req, crow::response &res);
bool parseJsonBody(const crow::request &req, crow::response &res, nlohmann::json &request_body);
std::string getCreateTime();

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

// 响应帮助类
class ResponseHelper
{
public:
    static crow::response success(const crow::request &req, const nlohmann::json &data)
    {
        crow::response res(200, data.dump());
        initializeCORS(req, res);
        return res;
    }

    static crow::response created(const crow::request &req, const nlohmann::json &data)
    {
        crow::response res(201, data.dump());
        initializeCORS(req, res);
        return res;
    }

    static crow::response error(const crow::request &req, const std::string &message)
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(400, error_data.dump());
        initializeCORS(req, res);
        return res;
    }

    static crow::response unauthorized(const crow::request &req, const std::string &message = "Unauthorized")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(401, error_data.dump());
        initializeCORS(req, res);
        return res;
    }

    static crow::response notFound(const crow::request &req, const std::string &message = "Not Found")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(404, error_data.dump());
        initializeCORS(req, res);
        return res;
    }

    static crow::response validation(const crow::request &req, const std::string &message = "Validation failed")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(422, error_data.dump());
        initializeCORS(req, res);
        return res;
    }

    static crow::response system_error(const crow::request &req, const std::string &message = "Internal Server Error")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(500, error_data.dump());
        initializeCORS(req, res);
        return res;
    }

    static crow::response unavailable(const crow::request &req, const std::string &message = "Service Unavailable")
    {
        nlohmann::json error_data;
        error_data["error"] = message;
        error_data["success"] = false;
        crow::response res(503, error_data.dump());
        initializeCORS(req, res);
        return res;
    }

    static crow::response custom(const crow::request &req, int status, const nlohmann::json &data)
    {
        crow::response res(status, data.dump());
        initializeCORS(req, res);
        return res;
    }
};


using CrowApp = crow::Crow<CorsMiddleware>;