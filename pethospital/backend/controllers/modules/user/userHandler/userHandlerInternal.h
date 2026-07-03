#pragma once

// userHandler 各分部实现（userHandler.cpp / userHandlerAuth.cpp）共享的私有辅助。
// 原在 userHandler.cpp 的匿名命名空间，因用户名拆分同时被 getUserData(资料域) 与
// userLogin(登录域) 使用、而这两者已拆到不同 .cpp，故上提为共享 inline，避免重复。
#include "userHandler.h" // 传递引入 clean_string / nlohmann::json 等

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// 按分隔符 "·" 把用户名拆成 姓 / 中间名 / 名。
inline nlohmann::json splitUserName(const std::string &rawName)
{
    const std::string delimiter = "·";
    std::vector<std::string> parts;
    std::string remaining = clean_string(rawName);
    std::size_t position = 0;

    while ((position = remaining.find(delimiter)) != std::string::npos)
    {
        parts.push_back(clean_string(remaining.substr(0, position)));
        remaining.erase(0, position + delimiter.length());
    }
    parts.push_back(clean_string(remaining));

    const std::string lastName = parts.size() > 0 ? parts[0] : "";
    const std::string middleName = parts.size() > 2 ? parts[1] : "";
    const std::string firstName = parts.size() > 2 ? parts[2] : (parts.size() > 1 ? parts[1] : "");

    return {
        {"lastName", lastName},
        {"middleName", middleName},
        {"firstName", firstName},
    };
}

// 将用户名拆分为姓、名和中间名，并添加到目标JSON对象中。
inline void appendUserNameParts(nlohmann::json &target, const std::string &rawName)
{
    const nlohmann::json nameParts = splitUserName(rawName);
    target["lastName"] = nameParts["lastName"];
    target["middleName"] = nameParts["middleName"];
    target["firstName"] = nameParts["firstName"];
}
