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
#include <curl/curl.h>
#include <fstream>
#include <map>

class Verify
{
private:
    std::string Emailaddress;
    std::string Code;

    // SMTP配置
    std::string smtp_server;
    int smtp_port;
    std::string smtp_username;
    std::string smtp_password;

     // 验证码过期时间（秒）
    static int expiration_seconds;
    
    // 存储验证码及其创建时间的结构
    struct CodeInfo {
        std::string code;
        std::chrono::time_point<std::chrono::steady_clock> creation_time;
    };
    
    // 存储所有验证码的静态容器
    static std::unordered_map<std::string, CodeInfo> code_storage;
    static std::mutex storage_mutex;

public:
    Verify(std::string emailaddress);

    // Base64编码函数
    std::string base64_encode(const std::string &input);

    // 判断验证邮箱条件
    bool VerifyEmailAddress(std::string emailaddress);

    // 判断验证密码条件
    bool VerifyPassword(std::string password);

    // 创建验证码
    std::string CreateVerify();

    // 获得邮箱地址
    std::string GetEmailAddress()
    {
        return Emailaddress;
    }

    // 获得验证码
    std::string GetVerifyCode()
    {
        return Code;
    }

    // 发送验证码
    void SendVerify(std::string emailaddress,const std::string code = "", std::promise<bool>* promise = nullptr);

    // 设置SMTP配置
    void SetSMTPConfig(std::string server, int port, std::string username, std::string password);

    // 从环境变量加载配置
    bool LoadConfigFromEnv();

    // 从配置文件加载配置
    bool LoadConfigFromFile(const std::string &configPath = "../../../../smtp_config.conf");

    // 验证码存储和管理相关静态方法
    static void StoreCode(const std::string& email, const std::string& code);
    static bool ValidateCode(const std::string& email, const std::string& code);
    static void CleanupExpiredCodes();
    static void SetExpirationTime(int seconds);

    // 添加用于调试的公共访问方法
    static std::unordered_map<std::string, CodeInfo> GetCodeStorage();
    static int GetExpirationSeconds();
};