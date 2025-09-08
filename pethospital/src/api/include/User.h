#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

// 用户结构体定义
struct User
{
    int id;                    // 用户ID
    std::string name;          // 用户名
    std::string password;      // 密码
    std::string phone;         // 手机号 // ← 注意：此字段包含国际区号，如 +86，不应被清理
    std::string email;         // 邮箱
    boost::gregorian::date birthday;  // 使用 boost::date_time
    std::string address_id;    // 地址ID
    std::string address;       // 地址

    // 构造函数
    User() : birthday(boost::gregorian::date(1970, 1, 1)) {}

};

// 函数声明
std::string clean_string(const std::string &input);
std::string format_date(const std::tm &tm);
std::string normalizeDate(const std::string& date_str);
void to_json(nlohmann::json &j, const User &user);
void from_json(const nlohmann::json &j, User &user);