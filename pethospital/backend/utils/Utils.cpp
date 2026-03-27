#include "Utils.h"

std::tm safeLocalTime(std::time_t time_value)
{
    std::tm tm_now{};
#ifdef _WIN32
    localtime_s(&tm_now, &time_value);
#else
    localtime_r(&time_value, &tm_now);
#endif
    return tm_now;
}

// 获得创建时间
std::string getCreateTime()
{
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now = safeLocalTime(time_now);
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    std::string creation_time = oss.str();

    return creation_time;
}

// 判断响应结果函数
void ProcessHandlerResponse(const crow::request &req, crow::response &res, crow::response &handlerResponse)
{
    (void)req;
    res = std::move(handlerResponse);
}

// 添加一个更严格的UTF-8字符串清理函数
std::string clean_string(const std::string &input)
{
    if (input.empty())
        return "";

    std::string result;
    result.reserve(input.length());

    for (size_t i = 0; i < input.length();)
    {
        unsigned char c = static_cast<unsigned char>(input[i]);

        // 特别处理 YYYY-MM-DD 格式的日期
        if (i + 9 < input.length() &&
            input[i] >= '0' && input[i] <= '9' &&
            input[i + 1] >= '0' && input[i + 1] <= '9' &&
            input[i + 2] >= '0' && input[i + 2] <= '9' &&
            input[i + 3] >= '0' && input[i + 3] <= '9' &&
            input[i + 4] == '-' &&
            input[i + 5] >= '0' && input[i + 5] <= '9' &&
            input[i + 6] >= '0' && input[i + 6] <= '9' &&
            input[i + 7] == '-' &&
            input[i + 8] >= '0' && input[i + 8] <= '9' &&
            input[i + 9] >= '0' && input[i + 9] <= '9')
        {
            // 匹配 YYYY-MM-DD 格式
            result += input.substr(i, 10);
            i += 10;
            continue;
        }

        // 处理单字节ASCII字符
        if (c <= 127)
        {
            // 只保留可打印的ASCII字符和一些安全的控制字符
            if ((c >= 32 && c <= 126) || c == '\t' || c == '\n' || c == '\r')
            {
                result += c;
            }
            i++;
        }
        // 处理UTF-8多字节序列开始字节
        else if (c >= 192 && c <= 223)
        {
            // 2字节UTF-8序列
            if (i + 1 < input.length() &&
                (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80)
            {
                // 有效的UTF-8序列，保留整个序列
                result += input[i];
                result += input[i + 1];
                i += 2;
            }
            else
            {
                // 无效序列，跳过
                i++;
            }
        }
        else if (c >= 224 && c <= 239)
        {
            // 3字节UTF-8序列
            if (i + 2 < input.length() &&
                (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80 &&
                (static_cast<unsigned char>(input[i + 2]) & 0xC0) == 0x80)
            {
                // 有效的UTF-8序列，保留整个序列
                result += input[i];
                result += input[i + 1];
                result += input[i + 2];
                i += 3;
            }
            else
            {
                // 无效序列，跳过
                i++;
            }
        }
        else if (c >= 240 && c <= 247)
        {
            // 4字节UTF-8序列
            if (i + 3 < input.length() &&
                (static_cast<unsigned char>(input[i + 1]) & 0xC0) == 0x80 &&
                (static_cast<unsigned char>(input[i + 2]) & 0xC0) == 0x80 &&
                (static_cast<unsigned char>(input[i + 3]) & 0xC0) == 0x80)
            {
                // 有效的UTF-8序列，保留整个序列
                result += input[i];
                result += input[i + 1];
                result += input[i + 2];
                result += input[i + 3];
                i += 4;
            }
            else
            {
                // 无效序列，跳过
                i++;
            }
        }
        else
        {
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
    oss << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900)    // 年份
        << '-' << std::setfill('0') << std::setw(2) << (tm.tm_mon + 1) // 月份
        << '-' << std::setfill('0') << std::setw(2) << tm.tm_mday;     // 日期
    return oss.str();
}

// 辅助函数：标准化日期格式
std::string normalizeDate(const std::string &date_str)
{
    if (date_str.empty())
        return "1970-01-01";

    try
    {
        boost::gregorian::date parsed_date;

        // 检查是否已经是 YYYY-MM-DD 格式
        if (date_str.length() == 10 && date_str[4] == '-' && date_str[7] == '-')
        {
            return date_str; // 已经是正确格式
        }

        // 尝试解析多种格式
        std::stringstream ss(date_str);

        // 尝试 %Y-%b-%d 格式 (例如: 2003-Sep-20)
        ss.clear();
        ss.str(date_str);
        ss.imbue(std::locale(std::locale::classic(), new boost::gregorian::date_input_facet("%Y-%b-%d")));
        ss >> parsed_date;

        if (!parsed_date.is_not_a_date())
        {
            std::ostringstream oss;
            oss << parsed_date;
            return oss.str();
        }

        // 如果以上方法都不行，返回默认日期
        return "1970-01-01";
    }
    catch (...)
    {
        return "1970-01-01";
    }
}

// 将boost::posix_time转换为MySQL datetime字符串格式 (YYYY-MM-DD HH:mm:ss)
std::string formatDateTime(const boost::posix_time::ptime& pt)
{
    std::ostringstream oss;
    boost::posix_time::time_facet* facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S");
    oss.imbue(std::locale(std::locale::classic(), facet));
    oss << pt;
    return oss.str();
}

// 只提取日期部分 YYYY-MM-DD
std::string formatDateOnly(const boost::posix_time::ptime& pt)
{
    return boost::gregorian::to_iso_extended_string(pt.date());
}

// 只提取时间部分 HH:MM:SS
std::string formatTimeOnly(const boost::posix_time::ptime& pt)
{
    std::ostringstream oss;
    boost::posix_time::time_duration time = pt.time_of_day();
    boost::posix_time::time_facet* facet = new boost::posix_time::time_facet("%H:%M:%S");
    oss.imbue(std::locale(std::locale::classic(), facet));
    oss << time;
    return oss.str();
}

// 加载环境变量文件
bool loadEnvironmentFile(const std::string &envFilePath)
{
    std::ifstream file(envFilePath);
    if (!file.is_open())
    {
        std::cerr << "Cannot open environment file: " << envFilePath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#' || line[0] == '/')
        {
            continue;
        }

        // 查找等号
        size_t pos = line.find('=');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // 设置环境变量
            setenv(key.c_str(), value.c_str(), 1);
        }
    }

    file.close();
    return true;
}

// 获取环境变量，带默认值
std::string getEnvVar(const std::string &name, const std::string &defaultValue)
{
    const char *value = getenv(name.c_str());
    return value ? std::string(value) : defaultValue;
}

// 在main函数开始时调用
void initializeEnvironment()
{
    std::string projectRoot = "/Users/yanghang/Code/PetManager";
    std::string envFile = projectRoot + "/.env";

    if (loadEnvironmentFile(envFile))
    {
        std::cout << "Environment variables loaded successfully" << std::endl;
    }
    else
    {
        std::cout << "Failed to load environment file" << std::endl;
    }
}
