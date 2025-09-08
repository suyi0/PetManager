#include "../include/User.h"

// ... existing code ...

// 添加一个更严格的UTF-8字符串清理函数
std::string clean_string(const std::string &input)
{
    if (input.empty())
        return "";

    std::string result;
    result.reserve(input.length());

    for (size_t i = 0; i < input.length(); ) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        
        // 特别处理 YYYY-MM-DD 格式的日期
        if (i + 9 < input.length() && 
            input[i] >= '0' && input[i] <= '9' &&
            input[i+1] >= '0' && input[i+1] <= '9' &&
            input[i+2] >= '0' && input[i+2] <= '9' &&
            input[i+3] >= '0' && input[i+3] <= '9' &&
            input[i+4] == '-' &&
            input[i+5] >= '0' && input[i+5] <= '9' &&
            input[i+6] >= '0' && input[i+6] <= '9' &&
            input[i+7] == '-' &&
            input[i+8] >= '0' && input[i+8] <= '9' &&
            input[i+9] >= '0' && input[i+9] <= '9') {
            // 匹配 YYYY-MM-DD 格式
            result += input.substr(i, 10);
            i += 10;
            continue;
        }
        
        // 处理单字节ASCII字符
        if (c <= 127) {
            // 只保留可打印的ASCII字符和一些安全的控制字符
            if ((c >= 32 && c <= 126) || c == '\t' || c == '\n' || c == '\r') {
                result += c;
            }
            i++;
        }
        // 处理UTF-8多字节序列开始字节
        else if (c >= 192 && c <= 223) {
            // 2字节UTF-8序列
            if (i + 1 < input.length() && 
                (static_cast<unsigned char>(input[i+1]) & 0xC0) == 0x80) {
                // 有效的UTF-8序列，保留整个序列
                result += input[i];
                result += input[i+1];
                i += 2;
            } else {
                // 无效序列，跳过
                i++;
            }
        }
        else if (c >= 224 && c <= 239) {
            // 3字节UTF-8序列
            if (i + 2 < input.length() && 
                (static_cast<unsigned char>(input[i+1]) & 0xC0) == 0x80 &&
                (static_cast<unsigned char>(input[i+2]) & 0xC0) == 0x80) {
                // 有效的UTF-8序列，保留整个序列
                result += input[i];
                result += input[i+1];
                result += input[i+2];
                i += 3;
            } else {
                // 无效序列，跳过
                i++;
            }
        }
        else if (c >= 240 && c <= 247) {
            // 4字节UTF-8序列
            if (i + 3 < input.length() && 
                (static_cast<unsigned char>(input[i+1]) & 0xC0) == 0x80 &&
                (static_cast<unsigned char>(input[i+2]) & 0xC0) == 0x80 &&
                (static_cast<unsigned char>(input[i+3]) & 0xC0) == 0x80) {
                // 有效的UTF-8序列，保留整个序列
                result += input[i];
                result += input[i+1];
                result += input[i+2];
                result += input[i+3];
                i += 4;
            } else {
                // 无效序列，跳过
                i++;
            }
        }
        else {
            // 其他字节（可能是无效的UTF-8序列的一部分），跳过
            i++;
        }
    }

    return result;
}

// 辅助函数：将 std::tm 转换为 YYYY-MM-DD 格式的字符串
std::string format_date(const std::tm &tm)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900)  // 年份
        << '-' << std::setfill('0') << std::setw(2) << (tm.tm_mon + 1)  // 月份
        << '-' << std::setfill('0') << std::setw(2) << tm.tm_mday;      // 日期
    return oss.str();
}

// 辅助函数：标准化日期格式
std::string normalizeDate(const std::string& date_str) {
    if (date_str.empty()) return "1970-01-01";
    
    try {
        boost::gregorian::date parsed_date;
        
        // 检查是否已经是 YYYY-MM-DD 格式
        if (date_str.length() == 10 && date_str[4] == '-' && date_str[7] == '-') {
            return date_str; // 已经是正确格式
        }
        
        // 尝试解析多种格式
        std::stringstream ss(date_str);
        
        // 尝试 %Y-%b-%d 格式 (例如: 2003-Sep-20)
        ss.clear();
        ss.str(date_str);
        ss.imbue(std::locale(std::locale::classic(), new boost::gregorian::date_input_facet("%Y-%b-%d")));
        ss >> parsed_date;
        
        if (!parsed_date.is_not_a_date()) {
            std::ostringstream oss;
            oss << parsed_date;
            return oss.str();
        }
        
        // 如果以上方法都不行，返回默认日期
        return "1970-01-01";
    } catch (...) {
        return "1970-01-01";
    }
}

// 为 User 结构体添加 to_json 函数，用于序列化
void to_json(nlohmann::json &j, const User &user)
{
    // 确保所有字符串字段都是有效的 UTF-8
    j = nlohmann::json{
        {"id", user.id},
        {"name", clean_string(user.name)},
        {"phone", clean_string(user.phone)},
        {"email", clean_string(user.email)},
        {"birthday", boost::gregorian::to_iso_extended_string(user.birthday)},  // 转换为 ISO 格式
        {"address_id", clean_string(user.address_id)},
        {"address", clean_string(user.address)}
    };
}
// 为 User 结构体添加 from_json 函数，用于反序列化（如果需要的话）
void from_json(const nlohmann::json &j, User &user)
{
    j.at("id").get_to(user.id);
    j.at("name").get_to(user.name);
    j.at("password").get_to(user.password);
    j.at("phone").get_to(user.phone);
    j.at("email").get_to(user.email);

    // 修复日期字段的反序列化
    std::string birthday_str = j.at("birthday").get<std::string>();
    user.birthday = boost::gregorian::from_simple_string(birthday_str);

    j.at("address_id").get_to(user.address_id);
    j.at("address").get_to(user.address);
}
