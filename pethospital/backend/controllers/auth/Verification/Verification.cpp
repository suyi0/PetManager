#include "Verification.h"

// 初始化静态成员
std::unordered_map<std::string, Verify::CodeInfo> Verify::code_storage; // 使用unordered_map存储验证码
std::mutex Verify::storage_mutex;
int Verify::expiration_seconds = 300; // 默认5分钟过期时间

// 添加获取当前日期的辅助函数
std::string Verify::getCurrentDate() {
    auto now = std::chrono::system_clock::now();
    std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
    
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S %z", std::gmtime(&time_t_now));
    return std::string(buffer);
}

// 获取验证码存储的公共方法
std::unordered_map<std::string, Verify::CodeInfo> Verify::GetCodeStorage()
{
    std::lock_guard<std::mutex> lock(storage_mutex);
    return code_storage;
}

// 获取过期时间的公共方法
int Verify::GetExpirationSeconds()
{
    return expiration_seconds;
}

// 添加base64编码函数（在其他函数之前）
std::string Verify::base64_encode(const std::string &input)
{
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;

    int val = 0, valb = -6;
    for (unsigned char c : input)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            encoded.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
        encoded.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (encoded.size() % 4)
        encoded.push_back('=');
    return encoded;
}

Verify::Verify(std::string emailaddress)
{
    this->Emailaddress = emailaddress;
}

bool Verify::VerifyEmailAddress(std::string emailaddress)
{
    // 正确的邮箱验证正则表达式
    std::regex emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    return std::regex_match(emailaddress, emailRegex);
}

bool Verify::VerifyPassword(std::string password)
{
    std::regex strongPasswordRegex("^(?=.*[a-z])(?=.*[A-Z])(?=.*\\d)(?=.*[@$!%*?&])[A-Za-z\\d@$!%*?&]{8,}$"); // 强密码正则
    std::regex passwordRegex("^{6,}$");                                                                       // 6以上位密码 - 密码正则
    return std::regex_match(password, strongPasswordRegex) || std::regex_match(password, passwordRegex);
}

std::string Verify::CreateVerify()
{
    // 生成验证码
    const std::string codeset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const int codesetSize = codeset.size() - 1;
    char codeBuffer[7]; // 6位验证码 + 1位结束符

    // 初始化随机数种子
    srand(time(0));

    // 生成6位随机验证码
    for (int i = 0; i < 6; ++i)
    {
        codeBuffer[i] = codeset[rand() % codesetSize];
    }
    codeBuffer[6] = '\0'; // 字符串结束符

    this->Code = std::string(codeBuffer);

    // 存储验证码及其创建时间
    StoreCode(this->Emailaddress, this->Code);

    return this->Code;
}

// 设置邮件服务器的配置
void Verify::SetSMTPConfig(std::string host, int port, std::string user, std::string password, std::string sender)
{
    smtp_host = host;       // SMTP服务器地址（如smtp.gmail.com）
    smtp_port = port;         // SMTP端口号（如587或465）
    smtp_user = user;         // 发送方邮箱地址
    smtp_password = password; // 邮箱密码或应用专用密码
    smtp_sender = sender;     // 发件人邮箱名称
}

// 从环境变量中加载配置
bool Verify::LoadConfigFromEnv()
{
    // 从环境变量加载配置
    const char *smtp_host = getenv("SMTP_HOST");
    const char *smtp_port = getenv("SMTP_PORT");
    const char *smtp_user = getenv("SMTP_USER");
    const char *smtp_password = getenv("SMTP_PASS");
    const char *smtp_sender = getenv("SMTP_SENDER");

    if (smtp_host && smtp_port && smtp_user && smtp_password && smtp_sender) // 所有配置项都存在
    {
        SetSMTPConfig(std::string(smtp_host), std::stoi(smtp_port), std::string(smtp_user), std::string(smtp_password), std::string(smtp_sender));
        return true;
    }
    return false;
}

// 添加一个自定义的读取函数
static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
    std::string *payload = (std::string *)userp;
    size_t room = size * nmemb;
    size_t len = payload->length();

    if (len > room)
        return 0;

    memcpy(ptr, payload->c_str(), len);
    payload->clear(); // 确保只发送一次
    return len;
}

void Verify::SendVerify(std::string emailaddress, std::string code, std::promise<bool> *promise)
{
    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *recipients = NULL;
    std::string payload_text;

    if (!this->LoadConfigFromEnv())
    {
        std::cerr << "Missing required SMTP environment variables: SMTP_HOST, SMTP_PORT, SMTP_USER, SMTP_PASS, SMTP_SENDER"
                  << std::endl;
        if (promise)
        {
            promise->set_value(false);
        }
        return;
    }

    std::cout << "SMTP配置加载成功" << std::endl;

    // 检查SMTP配置是否已正确设置
    if (smtp_host.empty() || smtp_port <= 0 || smtp_port > 65535)
    {
        fprintf(stderr, "Invalid SMTP configuration: server=%s, port=%d\n", smtp_host.c_str(), smtp_port);
        if (promise)
        {
            promise->set_value(false);
        }
        return;
    }

    // 构建邮件内容（符合RFC 2822标准）
    std::cout << "准备发送验证码: " << code << " 到邮箱: " << emailaddress << std::endl;
    // 修改：添加发件人名字
    payload_text = "To: " + emailaddress + ">\r\n";
    // 使用 "姓名 <邮箱>" 格式设置发件人
    payload_text += "From: \"YH\" <" + smtp_sender + ">\r\n";
    payload_text += "Subject: 验证码\r\n";
    payload_text += "Date: " + getCurrentDate() + "\r\n";
    payload_text += "Content-Type: text/plain; charset=UTF-8\r\n";
    payload_text += "MIME-Version: 1.0\r\n";
    payload_text += "\r\n";
    payload_text += "您好，\r\n";
    payload_text += "您的验证码是: " + code + "\r\n";
    payload_text += "此验证码将在5分钟内过期。\r\n";
    payload_text += "如果您没有请求此验证码，请忽略此邮件。\n";
    payload_text += "\r\n.\r\n"; // SMTP 结束符

    std::cout << "邮件内容长度: " << payload_text.length() << std::endl;

    curl = curl_easy_init();
    if (curl)
    {
        // 根据端口号选择合适的协议
        std::string smtp_url;
        if (smtp_port == 465)
        {
            smtp_url = "smtps://" + smtp_host + ":" + std::to_string(smtp_port);
        }
        else
        {
            smtp_url = "smtp://" + smtp_host + ":" + std::to_string(smtp_port);
        }
        curl_easy_setopt(curl, CURLOPT_URL, smtp_url.c_str());

        // 强制使用IPv4（QQ邮箱通常对IPv4支持更好）
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

        // 设置用户名和密码
        curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_user.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_password.c_str());

        // 对于465端口，强制使用SSL
        if (smtp_port == 465)
        {
            curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        }
        else
        {
            // 对于587端口，使用STARTTLS
            curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        }
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

        // 设置SSL验证选项（在开发环境中可以暂时禁用，生产环境中应该启用）
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        // 后期完成后要申请CA证书，完成SSL验证

        // 设置发件人
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, smtp_user.c_str());

        // 添加收件人
        recipients = curl_slist_append(recipients, emailaddress.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        // 设置邮件内容
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source); // 使用字符串作为邮件内容
        curl_easy_setopt(curl, CURLOPT_READDATA, &payload_text);      // 指定邮件内容
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);                   // 设置上传模式

        // 启用详细输出以便调试
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        // 设置连接超时（毫秒）
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

        // 设置AUTH选项
        curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");

        res = curl_easy_perform(curl); // 执行curl

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            if (promise)
            {
                promise->set_value(false); // 设置promise结果为失败
            }
        }
        else
        {
            if (promise)
            {
                promise->set_value(true); // 设置promise结果为成功
            }
        }

        curl_slist_free_all(recipients); // 释放资源
        curl_easy_cleanup(curl);         // 清理curl
    }
    else
    {
        if (promise)
        {
            promise->set_value(false); // curl初始化失败
        }
    }
}

// 存储验证码
void Verify::StoreCode(const std::string &email, const std::string &code)
{
    std::lock_guard<std::mutex> lock(storage_mutex);
    CodeInfo info;
    info.code = code;
    info.creation_time = std::chrono::steady_clock::now();
    code_storage[email] = info;
}

// 验证验证码
bool Verify::ValidateCode(const std::string &email, const std::string &code)
{
    std::lock_guard<std::mutex> lock(storage_mutex);
    // lock_guard是一个 RAII（Resource Acquisition Is Initialization）风格的锁管理器，它在构造时自动获取锁，在析构时自动释放锁。

    auto it = code_storage.find(email);
    if (it == code_storage.end())
    {
        return false; // 邮箱不存在
    }

    // 检查是否过期
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.creation_time).count();
    // now - it->second.creation_time 计算两个时间点之间的时间差，返回一个duration对象
    // chrono::duration_cast<chrono::seconds> 将时间差转换为秒数
    // .count() 获取具体的数值（秒数）

    if (elapsed > expiration_seconds)
    {
        // 验证码已过期，删除它
        code_storage.erase(it);
        return false;
    }

    // 检查验证码是否匹配
    if (it->second.code == code && elapsed <= expiration_seconds)
    {
        // 验证成功，删除验证码（一次性使用）
        code_storage.erase(it);
        return true;
    }

    return false;
}

// 清理过期验证码
void Verify::CleanupExpiredCodes()
{
    std::lock_guard<std::mutex> lock(storage_mutex);

    auto now = std::chrono::steady_clock::now();
    for (auto it = code_storage.begin(); it != code_storage.end();)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.creation_time).count();
        if (elapsed > expiration_seconds)
        {
            it = code_storage.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// 设置过期时间（秒）
void Verify::SetExpirationTime(int seconds)
{
    expiration_seconds = seconds;
}
