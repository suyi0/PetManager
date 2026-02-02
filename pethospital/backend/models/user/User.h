#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "../../controllers/auth/Verification/Verification.h"


//使用智能指针构建对象时
//      char* raw_data;        // 原始指针 - 需要手动管理
//      FILE* file_handle;     // 文件句柄 - 需要手动关闭
//其他资源智能指针可以自动释放

// 用户类定义
class User
{
    int id;                    // 用户ID
    int type_id;               // 用户类型ID
    std::string name;          // 用户名
    std::string password;      // 密码
    std::string phone;         // 手机号 // ← 注意：此字段包含国际区号，如 +86，不应被清理
    std::string email;         // 邮箱
    boost::gregorian::date birthday;  // 使用 boost::date_time
    int address_id;    // 地址ID
    std::string address;       // 地址
    std::string head_image;    // 头像
    bool token = false;         // 令牌

public:
    // 构造函数
    User() : birthday(boost::gregorian::date(1970, 1, 1)) {}

    void to_json(nlohmann::json &j);
    void from_json(const nlohmann::json &j);

    // 设置属性方法

    void setID(int id)                                         // 设置用户ID
    {
        this->id = id;
    }
    void setTypeID(int type_id)                                // 设置用户类型ID
    {
        this->type_id = type_id;
    }
    void setName(const std::string &name)                      // 设置用户名
    {
        this->name = name;
    }
    void setPassword(const std::string &password)               // 设置密码
    {
        this->password = password;
    }
    void setPhone(const std::string &phone)                    // 设置手机号
    {
        this->phone = phone;
    }
    void setEmail(const std::string &email)                    // 设置邮箱
    {
        this->email = email;
    }
    void setBirthday(const boost::gregorian::date &birthday)   // 设置生日
    {
        this->birthday = birthday;
    }
    void setAddressID(int &address_id)                          // 设置地址ID
    {
        this->address_id = address_id;
    }
    void setAddress(const std::string &address)                // 设置地址
    {
        this->address = address;
    }
    void setHeadImage(const std::string &head_image)           // 设置头像
    {
        this->head_image = head_image;
    }
    void setToken(bool token)                                   // 设置令牌
    {
        this->token = token;
    }
    // 获取属性方法
    int getID()                           // 获取用户ID
    { 
        return id;
    }
    int getTypeID()                       // 获取用户类型ID
    {
        return type_id;
    }
    std::string getName()                 // 获取用户名
    {
        return name;
    }
    std::string getPassword()             // 获取密码
    {
        return password;
    }
    std::string getPhone()                // 获取手机号
    {
        return phone;
    }
    std::string getEmail()                // 获取邮箱
    {
        return email;
    }
    boost::gregorian::date getBirthday()  // 获取生日
    {
        return birthday;
    }
    int getAddressID()            // 获取地址ID
    {
        return address_id;
    }
    std::string getAddress()              // 获取地址
    {
        return address;
    }
    std::string getHeadImage()            // 获取头像
    {
        return head_image;
    }

    bool getToken()                       // 获取令牌
    {
        return token;
    }

};

std::string clean_string(const std::string &input);
std::string format_date(const std::tm &tm);
std::string normalizeDate(const std::string& date_str);