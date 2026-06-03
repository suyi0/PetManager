#include "User.h"

// 为 User 类添加 to_json 函数，用于序列化
void User::to_json(nlohmann::json &j)
{
    // 确保所有字符串字段都是有效的 UTF-8
    j = nlohmann::json{
        {"id", this->id},
        {"type_id", this->type_id},
        {"name", clean_string(this->name)},
        {"phone", this->phone},
        {"email", this->email},
        {"birthday", boost::gregorian::to_iso_extended_string(this->birthday)},  // 转换为 ISO 格式
        {"address", clean_string(this->address)},
        {"head_image", clean_string(this->head_image)},
        {"doctor_specialty", clean_string(this->doctor_specialty)}
    };
}
// 为 User 类添加 from_json 函数，用于反序列化（如果需要的话）
void User::from_json(const nlohmann::json &j)
{
    j.at("id").get_to(this->id);
    j.at("type_id").get_to(this->type_id);
    j.at("name").get_to(this->name);
    j.at("phone").get_to(this->phone);
    j.at("email").get_to(this->email);
    // 修复日期字段的反序列化
    std::string birthday_str = j.at("birthday").get<std::string>();
    this->birthday = boost::gregorian::from_simple_string(birthday_str);
    j.at("address").get_to(this->address);
    j.at("head_image").get_to(this->head_image);
    j.at("doctor_specialty").get_to(this->doctor_specialty);
}
