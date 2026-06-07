#ifndef AUTH_IDENTIFIER_UTILS_H
#define AUTH_IDENTIFIER_UTILS_H

#include <algorithm>
#include <cctype>
#include <regex>
#include <string>

#include "Utils.h"

// 验证邮箱格式
inline bool isValidEmailFormat(const std::string &email)
{
    static const std::regex email_pattern(
        R"(^[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}$)");
    return std::regex_match(email, email_pattern);
}

// 验证手机号格式
inline bool isValidPhoneFormat(const std::string &phone)
{
    static const std::regex phone_pattern(R"(^(?:\+86)?1[3-9]\d{9}$)");
    return std::regex_match(phone, phone_pattern);
}

// 验证密码格式：至少 6 位，且同时包含字母和数字。
inline bool isValidPasswordFormat(const std::string &password)
{
    static const std::regex password_pattern(R"(^(?=.*[A-Za-z])(?=.*\d).{6,}$)");
    return std::regex_match(password, password_pattern);
}

// 标准化手机号（去除空格，添加国家码）
inline std::string normalizePhoneIdentifier(std::string phoneValue)
{
    phoneValue = clean_string(phoneValue);
    phoneValue.erase(
        std::remove_if(
            phoneValue.begin(),
            phoneValue.end(),
            [](unsigned char ch) { return std::isspace(ch) != 0; }),
        phoneValue.end());

    if (phoneValue.rfind("+86", 0) == 0)
    {
        return phoneValue;
    }

    if (!phoneValue.empty() && phoneValue.front() != '+')
    {
        return std::string("+86") + phoneValue;
    }

    return phoneValue;
}

#endif
