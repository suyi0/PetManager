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

// 固定时间字符串比较，适合验证码、签名、Token 摘要等敏感值比较。
// left - 接收用户输入值
// right - 接收服务器端存储的值
bool constantTimeEquals(const std::string &left, const std::string &right)
{
    if (left.size() != right.size())
    {
        return false;
    }

    unsigned char diff = 0; // 存储比较结果
    // 1（ & ）按位与：两个位都为 1 时结果为 1，否则为 0。
    // 2（ | ）按位或：两个位中至少一个为 1 时结果为 1，否则为 0。
    // 3（ ^ ）按位异或：两个位不同时结果为 1，相同时为 0。
    // 4（ ~ ）按位取反：将每个位取反（0 变 1，1 变 0）。
    // 5（ << ）左移：所有位向左移动，右侧补 0。相当于乘以 2 的幂。
    // 6（ >> ）右移：所有位向右移动，符号位扩展（有符号数）或补 0（无符号数）。相当于除以 2 的幂。
    for (size_t i = 0; i < left.size(); ++i)
    {
        // 当left和right相等时，最终diff = 0
        // 当left和right不相等时，最终diff != 0
        diff |= static_cast<unsigned char>(left[i] ^ right[i]);

        // 在异或运算中：
        // 有符号扩展：0xFFFFFFFF ^ 0x00000000 = 0xFFFFFFFF
        // 无符号扩展：0x000000FF ^ 0x00000000 = 0x000000FF

        // 有符号情况
        // char类型字符面量的二进制为8位，当使用进行运算时，自动转换为int类型字符面量的32位，
        // 符号扩展规则：用符号位（最高位）填充所有新增的高位
        // 如：char 值 255 (11111111) → 提升为 int 后是 0xFFFFFFFF

        // 无符号情况
        // char类型字符面量的二进制为8位，当使用进行运算时，自动转换为int类型字符面量的32位，
        // 符号扩展规则：扩展时高位补 0
        // 如：unsigned char 值 255 (11111111) → 提升为 int 后是 0x000000FF
    }

    return diff == 0;
}

// 事务失败路径使用：回滚失败不覆盖原始业务错误或异常。
void rollbackTransactionQuietly(mysqlx::Session &session)
{
    try
    {
        session.sql("ROLLBACK").execute();
    }
    catch (...)
    {
    }
}

// 从请求 JSON 中读取字符串字段；字段不存在或为 null 时返回默认值。
// 如果前端传入数字、布尔等 JSON 值，则转成字符串，方便表单字段统一落库。
std::string getRequestString(const nlohmann::json &request_body, const std::string &key, const std::string &defaultValue)
{
    if (!request_body.contains(key) || request_body[key].is_null())
    {
        return defaultValue;
    }

    if (request_body[key].is_string())
    {
        return request_body[key].get<std::string>();
    }

    return request_body[key].dump();
}

std::string getRequestStringWithFallback(
    const nlohmann::json &request_body,
    const std::string &primaryKey,
    const std::string &fallbackKey,
    const std::string &DBValue)
{
    // 检查request_body是否存在key值，如果存在则返回request_body中的值，否则返回DBValue（数据库中的原值）
    if (request_body.contains(primaryKey))
    {
        return getRequestString(request_body, primaryKey, DBValue);
    }

    return getRequestString(request_body, fallbackKey, DBValue);
};

// 从请求 JSON 中读取数字字段；字段不存在或为 null 时返回默认值。
double getRequestDouble(
    const nlohmann::json &request_body,
    const std::string &key,
    double defaultValue)
{
    if (!request_body.contains(key) || request_body[key].is_null())
    {
        return defaultValue;
    }

    if (!request_body[key].is_number())
    {
        throw std::invalid_argument(key + " must be a number");
    }

    return request_body[key].get<double>();
}

// 优先读取 primaryKey，缺失时兼容 fallbackKey，适合接口字段重命名过渡期复用。
double getRequestDoubleWithFallback(
    const nlohmann::json &request_body,
    const std::string &primaryKey,
    const std::string &fallbackKey,
    double DBValue)
{
    if (request_body.contains(primaryKey))
    {
        return getRequestDouble(request_body, primaryKey, DBValue);
    }

    return getRequestDouble(request_body, fallbackKey, DBValue);
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
std::string formatDateTime(const boost::posix_time::ptime &pt)
{
    std::ostringstream oss;
    boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S");
    oss.imbue(std::locale(std::locale::classic(), facet));
    oss << pt;
    return oss.str();
}

// 只提取日期部分 YYYY-MM-DD
std::string formatDateOnly(const boost::posix_time::ptime &pt)
{
    return boost::gregorian::to_iso_extended_string(pt.date());
}

// 只提取时间部分 HH:MM:SS
std::string formatTimeOnly(const boost::posix_time::ptime &pt)
{
    std::ostringstream oss;
    boost::posix_time::time_duration time = pt.time_of_day();
    boost::posix_time::time_facet *facet = new boost::posix_time::time_facet("%H:%M:%S");
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

// 项目根目录的单一来源：优先环境变量 PROJECT_ROOT（部署机设置），
// 否则回退到开发机路径。原先散落在 Utils/userHandler 各处硬编码，现统一到此。
std::string getProjectRoot()
{
    return getEnvVar("PROJECT_ROOT", "/Users/yanghang/Code/PetManager");
}

// 服务监听端口：环境变量 SERVER_PORT，默认 8081；非法值回退默认。
int getServerPort()
{
    try
    {
        return std::stoi(getEnvVar("SERVER_PORT", "8081"));
    }
    catch (...)
    {
        return 8081;
    }
}

// 在main函数开始时调用。
// 依次尝试候选 .env 路径，命中即止：PROJECT_ROOT/.env → 当前工作目录 ./.env → 开发机回退。
// 注意：定位 .env 用的 PROJECT_ROOT 只能来自 ambient 环境变量（此时 .env 还没加载），
// 部署机通过 systemd Environment= 或 cwd 下的 .env 即可，不再依赖硬编码路径。
void initializeEnvironment()
{
    std::vector<std::string> candidates;
    if (const char *pr = getenv("PROJECT_ROOT"); pr && *pr)
    {
        candidates.push_back(std::string(pr) + "/.env");
    }
    candidates.push_back(".env");                                 // 当前工作目录
    candidates.push_back("/Users/yanghang/Code/PetManager/.env"); // 开发机回退

    for (const std::string &envFile : candidates)
    {
        if (loadEnvironmentFile(envFile))
        {
            std::cout << "Environment variables loaded from " << envFile << std::endl;
            return;
        }
    }
    std::cout << "No .env file loaded; relying on ambient environment." << std::endl;
}
