#pragma once
#include <stdio.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <csignal>
#include <future>
#include <regex>
#include <random>
#include <curl/curl.h>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>

// 密码认证
// 邮箱验证
// 手机验证码
// OAuth（第三方登录）
// JWT Token认证

// 验证类定义
class Verify
{
private:
    // 验证码过期时间（秒）
    static int expiration_seconds;

    // 存储验证码及其创建时间的结构
    struct CodeInfo
    {
        std::string code;
        std::chrono::time_point<std::chrono::steady_clock> creation_time;
    };

    // 存储所有验证码的静态容器
    static std::unordered_map<std::string, CodeInfo> code_storage;
    static std::mutex storage_mutex;

public:

    // Base64编码函数
    static std::string base64_encode(const std::string &input);

    // 创建邮箱验证码
    static std::string CreateEmailVerify(const std::string &email);

    // 创建手机验证码
    static std::string CreatePhoneVerify(const std::string &phone);

    // 获取当前日期
    static std::string getCurrentDate();

    // 发送验证码
    static void SendEmailVerify(const std::string &emailaddress, const std::string &code = "", std::promise<bool> *promise = nullptr);

    // 验证码存储和管理相关静态方法
    static void StoreCode(const std::string &email, const std::string &code);
    static bool ValidateCode(const std::string &email, const std::string &input_code);
    static void CleanupExpiredCodes();
    static void SetExpirationTime(int seconds);

    // 添加用于调试的公共访问方法
    static std::unordered_map<std::string, CodeInfo> GetCodeStorage();
    static int GetExpirationSeconds();
};
