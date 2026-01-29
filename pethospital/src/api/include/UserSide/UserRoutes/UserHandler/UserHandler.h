
#ifndef USERHANDLER_H
#define USERHANDLER_H 

#include <mysqlx/xdevapi.h>
#include <unordered_map>
#include <chrono>
#include "../../../../src/DatabaseManager/DatabaseManager.h"
#include "../../../Shared/Utils/Utils.h"
#include "nlohmann/json.hpp"
#include "../../Auth/Verification/Verification.h"
#include "../../Auth/UserEncrypt/Encrypt.h"
#include "../User/User.h"
#include "../GetAddress/GetAddress.h"

// 添加邮箱格式验证函数
bool isValidEmailFormat(const std::string &email);

class RateLimiter {
private:
    struct ClientInfo {
        std::chrono::steady_clock::time_point last_request;
        int request_count;
    };
    
    std::unordered_map<std::string, ClientInfo> client_requests;
    const int time_window_seconds = 5; // 时间窗口为5秒
    const int max_requests = 1; // 每个时间窗口内最多1次请求

public:
    bool is_allowed(const std::string& client_ip) {
        auto now = std::chrono::steady_clock::now();
        auto it = client_requests.find(client_ip);
        
        if (it == client_requests.end()) {
            // 第一次请求
            client_requests[client_ip] = {now, 1};
            return true;
        }
        
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.last_request
        ).count();
        
        if (elapsed >= time_window_seconds) {
            // 时间窗口已过，重置计数器
            it->second.last_request = now;
            it->second.request_count = 1;
            return true;
        } else {
            // 时间窗口内
            if (it->second.request_count < max_requests) {
                it->second.request_count++;
                return true;
            } else {
                // 达到最大请求数
                return false;
            }
        }
    }
};

class UserHandler {
private:
    DatabaseManagerInterface* dbManager;
    static RateLimiter rate_limiter; // 创建一个静态的RateLimiter对象
public:
    explicit UserHandler(DatabaseManagerInterface* dbManager) : dbManager(dbManager) {}
    // 添加一个方法来检查数据库管理器是否可用
    bool isDbManagerValid() const { return dbManager != nullptr; }

    crow::response userLogin(const crow::request& req); //  对应 "/api/user/login"

    crow::response userReadyVerification(const crow::request& req); //  对应 "/api/verification/ready"

    crow::response userCheckEmail(const crow::request& req); // 对应 "/api/user/check/email"

    crow::response userVerification(const crow::request& req); //   对应 "/api/verification/verify"

    crow::response userUpdate(const crow::request& req); // 对应 "/api/user/form"

    crow::response userUploadAvatar(const crow::request& req); // 对应 "/api/user/upload/avatar"

    crow::response upload(const crow::request& req, const std::string& filename); // 对应 "/uploads/<string>"

    crow::response getUserdata(const crow::request& req); // 对应 "/api/user/data"

};

#endif