#include "getAddress.h"

// 添加URL编码函数
std::string urlencode(const std::string &value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value)
    {
        // 保持字符不变的条件：字母、数字、-、_、.、~
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
        }
        // 空格编码为+
        else if (c == ' ')
        {
            escaped << '+';
        }
        // 其他字符编码为%XX格式
        else
        {
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }

    return escaped.str();
}
// 使用 cURL 发起 HTTP 请求(高德地图API Key)，获取地址的经纬度信息
std::string geocodeAddress(const std::string &address)
{
    const char *amap_key = std::getenv("AMAP_GEOCODE_KEY");
    if (amap_key == nullptr || std::string(amap_key).empty())
    {
        std::cerr << "AMAP_GEOCODE_KEY is not set" << std::endl;
        return "";
    }

    std::string url = "https://restapi.amap.com/v3/geocode/geo?key=" + urlencode(amap_key) +
                      "&address=" + urlencode(address);

    CURL *curl = curl_easy_init(); // 初始化 CURL* 句柄
    if (!curl)
        return "";

    std::string response;
    /*
        curl_easy_setopt(已初始化的CURL* 句柄,
                         libcurl 预定义的选项常量表示接下来要设置的是请求的 URL,
                         一个以 \0 结尾的 C 风格字符串（const char*），即实际要访问的 HTTP/HTTPS 地址)
    */
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // 设置请求的目标 URL
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    /*
        curl_easy_setopt(已初始化的CURL* 句柄,
                         libcurl 预定义的选项常量表示接下来要设置的是回调函数,
                         一个函数指针，指向一个用户定义的函数，这个函数将被 libcurl 调用来处理服务器响应的数据)
    */
    /*
        lambda函数 [](指向接收到的数据块, 每个元素的字节数, 元素的数量, 用户数据指针) -> 返回值类型
        这个函数会被 libcurl 调用，
        每当接收到服务器响应的数据时，libcurl 会将数据块的指针、每个元素的字节数、元素的数量以及用户数据指针传递给这个函数。
        函数的返回值实际处理的字节数（必须等于 size * nmemb，否则 libcurl 会报错）
    */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void *ptr, size_t size, size_t nmemb, void *userdata) -> size_t
                     {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb; }); // 注册回调函数，将响应数据写入字符串

    /*
        curl_easy_setopt(已初始化的CURL* 句柄,
                         libcurl 预定义的选项常量表示接下来要设置的是回调函数的用户数据,
                         一个指针，指向用户定义的数据结构，这个指针会被传递给上面设置的回调函数)
    */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response); // 将 &response（一个 std::string*）作为用户数据传递给之前设置的写回调函数（CURLOPT_WRITEFUNCTION）

    CURLcode res = curl_easy_perform(curl); // 执行 curl 请求
    curl_easy_cleanup(curl);                // 清理 curl 句柄

    if (res != CURLE_OK) // 检查 curl 请求是否成功
    {
        return "";
    }

    try
    {
        nlohmann::json j = nlohmann::json::parse(response);
        if (j["status"] == "1" && !j["geocodes"].empty())
        {
            /*
                j{
                    "status": "1",
                    "geocodes": [
                        {
                            "formatted_address": "北京市朝阳区xxx",
                            "province": "北京市",
                            "city": "北京市",
                            "district": "朝阳区",
                            // ... 其他地址组件字段
                        }
                    ]
                }
            */
            j["geocodes"][0]["geocode_source"] = "amap"; // 标记经纬度来源为高德
            return j.dump();                             // 返回完整响应，保持调用方按 status/geocodes 解析
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

    return "";
}
