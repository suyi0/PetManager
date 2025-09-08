#include "../include/GetAddress.h"

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
// 使用 cURL 发起 HTTP 请求(高德地图API Key)
std::string geocodeAddress(const std::string &address)
{
    std::string url = "https://restapi.amap.com/v3/geocode/geo?key=YOUR_KEY&address=" +
                      urlencode(address);

    CURL *curl = curl_easy_init();
    if (!curl)
        return "";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void *ptr, size_t size, size_t nmemb, void *userdata) -> size_t
                     {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(static_cast<char*>(ptr), size * nmemb);
        return size * nmemb; });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "Geocoding failed: " << curl_easy_strerror(res) << std::endl;
        return "";
    }

    try
    {
        nlohmann::json j = nlohmann::json::parse(response);
        if (j["status"] == "1" && !j["geocodes"].empty())
        {
            auto &geo = j["geocodes"][0];
            return geo.dump(); // 返回 JSON 字符串
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

    return "";
}

