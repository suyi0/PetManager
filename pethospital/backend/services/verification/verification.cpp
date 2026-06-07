#include "verification.h"
#include "../../utils/Utils.h"
#include <algorithm>
#include <cstring>

namespace
{
    struct SmtpConfig   // SMTP服务器配置
    {
        std::string host;
        int port = 0;
        std::string user;
        std::string password;
        std::string sender;
    };

    struct UploadStatus // 验证码存储结构体
    {
        const std::string *payload; // 指向上传数据的指针
        size_t bytesRead = 0;       // 已读取的字节数
    };

    // 检查环境变量是否启用
    bool isEnvEnabled(const char *value)
    {
        if (!value)
        {
            return false;
        }

        const std::string flag(value);
        return flag == "1" || flag == "true" || flag == "TRUE" || flag == "yes" || flag == "YES";
    }

    bool loadSmtpConfig(SmtpConfig &config) // 加载SMTP服务器配置
    {
        const char *smtpHost = getenv("SMTP_HOST");
        const char *smtpPort = getenv("SMTP_PORT");
        const char *smtpUser = getenv("SMTP_USER");
        const char *smtpPassword = getenv("SMTP_PASS");
        const char *smtpSender = getenv("SMTP_SENDER");

        if (!smtpHost || !smtpPort || !smtpUser || !smtpPassword || !smtpSender)
        {
            return false;
        }

        try
        {
            config.host = smtpHost;
            config.port = std::stoi(smtpPort);
            config.user = smtpUser;
            config.password = smtpPassword;
            config.sender = smtpSender;
            return true;
        }
        catch (const std::exception &)
        {
            return false;
        }
    }
}

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp);

// 初始化静态成员
// 使用unordered_map存储验证码, <std::string : 索引键>, <CodeInfo : 索引值>
std::unordered_map<std::string, Verify::CodeInfo> Verify::code_storage;
std::mutex Verify::storage_mutex;                                       // 用于保护code_storage的互斥锁
int Verify::expiration_seconds = 300;                                   // 默认5分钟过期时间

// 添加获取当前日期的辅助函数
std::string Verify::getCurrentDate()
{
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

// 生成邮箱验证码
std::string Verify::CreateEmailVerify(const std::string &email)
{
    static const std::string codeset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    // static：静态存储期，变量在程序生命周期内只初始化一次
    // thread_local：每个线程拥有独立副本，避免多线程竞争
    // std::mt19937：梅森旋转伪随机数生成器（高质量、常用）
    // std::random_device{}()：创建一个真随机设备并立即调用它获取种子
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<size_t> distribution(0, codeset.size() - 1);
    char codeBuffer[9]; // 6位验证码 + 1位结束符

    // 生成8位随机验证码
    for (int i = 0; i < 8; ++i)
    {
        codeBuffer[i] = codeset[distribution(generator)];
    }
    codeBuffer[8] = '\0'; // 字符串结束符

    std::string code(codeBuffer);

    // 存储验证码及其创建时间
    StoreCode(email, code);

    return code;
}

// 生成手机验证码
std::string Verify::CreatePhoneVerify(const std::string &phone)
{
    static const std::string codeset = "0123456789";
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<size_t> distribution(0, codeset.size() - 1);
    char codeBuffer[7];

    for (int i = 0; i < 6; ++i)
    {
        codeBuffer[i] = codeset[distribution(generator)];
    }
    codeBuffer[6] = '\0';

    std::string code(codeBuffer);

    StoreCode(phone, code);

    return code;
}

// 发送邮箱验证码
void Verify::SendEmailVerify(const std::string &emailaddress, const std::string &code, std::promise<bool> *promise)
{
    auto finish = [promise](bool success)   // 定义一个lambda函数，用于处理发送邮件的完成情况
    {
        if (promise)
        {
            promise->set_value(success);
        }
    };

    CURLcode res = CURLE_OK;
    CURL *curl = nullptr;
    struct curl_slist *recipients = nullptr;
    std::string payload_text;

    SmtpConfig smtpConfig;
    if (!loadSmtpConfig(smtpConfig)) // 检查SMTP配置是否已正确设置
    {
        finish(false);
        return;
    }

    // 构建邮件内容（符合RFC 2822标准）
    payload_text.reserve(512);
    payload_text = "To: <" + emailaddress + ">\r\n";
    // 使用 "姓名 <邮箱>" 格式设置发件人
    payload_text += "From: \"YH\" <" + smtpConfig.sender + ">\r\n";
    payload_text += "Subject: 验证码\r\n";
    payload_text += "Date: " + getCurrentDate() + "\r\n";
    payload_text += "Content-Type: text/plain; charset=UTF-8\r\n";
    payload_text += "MIME-Version: 1.0\r\n";
    payload_text += "\r\n";
    payload_text += "您好，\r\n";
    payload_text += "您的验证码是: " + code + "\r\n";
    payload_text += "此验证码将在5分钟内过期。\r\n";
    payload_text += "如果您没有请求此验证码，请忽略此邮件。\n";

    curl = curl_easy_init();
    if (curl)
    {
        // 根据端口号选择合适的协议
        std::string smtp_url;
        if (smtpConfig.port == 465)
        {
            smtp_url = "smtps://" + smtpConfig.host + ":" + std::to_string(smtpConfig.port);
        }
        else
        {
            smtp_url = "smtp://" + smtpConfig.host + ":" + std::to_string(smtpConfig.port);
        }
        curl_easy_setopt(curl, CURLOPT_URL, smtp_url.c_str());

        // 强制使用IPv4（QQ邮箱通常对IPv4支持更好）
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

        // 设置用户名和密码
        curl_easy_setopt(curl, CURLOPT_USERNAME, smtpConfig.user.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, smtpConfig.password.c_str());

        // 对于465端口，强制使用SSL
        if (smtpConfig.port == 465)
        {
            curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        }
        else
        {
            // 对于587端口，使用STARTTLS
            curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        }
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        // 设置发件人
        const std::string senderAddress = "<" + (smtpConfig.sender.empty() ? smtpConfig.user : smtpConfig.sender) + ">";
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, senderAddress.c_str());

        // 添加收件人
        const std::string recipientAddress = "<" + emailaddress + ">";
        recipients = curl_slist_append(recipients, recipientAddress.c_str());
        if (!recipients) // 添加收件人失败，及时释放curl
        {
            curl_easy_cleanup(curl);
            finish(false);
            return;
        }
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        // 设置邮件内容
        UploadStatus uploadStatus{&payload_text, 0};
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source); // 使用字符串作为邮件内容
        curl_easy_setopt(curl, CURLOPT_READDATA, &uploadStatus);      // 指定邮件内容
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);                   // 设置上传模式

        curl_easy_setopt(curl, CURLOPT_VERBOSE, isEnvEnabled(getenv("SMTP_CURL_VERBOSE")) ? 1L : 0L);

        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        // 设置AUTH选项
        curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");

        res = curl_easy_perform(curl); // 执行curl

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            finish(false);
        }
        else
        {
            finish(true);
        }

        curl_slist_free_all(recipients); // 释放资源
        curl_easy_cleanup(curl);         // 清理curl
    }
    else
    {
        finish(false); // curl初始化失败
    }
}

// 添加一个自定义的读取函数
static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
    auto *uploadStatus = static_cast<UploadStatus *>(userp);
    if (!uploadStatus || !uploadStatus->payload)
    {
        return 0;
    }

    const size_t bufferSize = size * nmemb;
    if (bufferSize == 0)
    {
        return 0;
    }

    const std::string &payload = *uploadStatus->payload;
    const size_t bytesLeft = payload.size() - uploadStatus->bytesRead;
    const size_t bytesToCopy = std::min(bufferSize, bytesLeft);

    if (bytesToCopy == 0)
    {
        return 0;
    }

    std::memcpy(ptr, payload.c_str() + uploadStatus->bytesRead, bytesToCopy);
    uploadStatus->bytesRead += bytesToCopy;
    return bytesToCopy;
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
bool Verify::ValidateCode(const std::string &email, const std::string &input_code)
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
    // std::chrono::duration_cast<chrono::seconds> 将时间差转换为秒数
    // now - it->second.creation_time 计算两个时间点之间的时间差，返回一个duration对象
    // .count() 获取具体的数值（秒数）

    if (elapsed > expiration_seconds)
    {
        // 验证码已过期，删除它
        code_storage.erase(it);
        return false;
    }

    // 检查验证码是否匹配
    if (constantTimeEquals(it->second.code, input_code) && elapsed <= expiration_seconds)
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
