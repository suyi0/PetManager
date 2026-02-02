#pragma once
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>

// 声明URL编码和地理编码函数
std::string urlencode(const std::string &value);
std::string geocodeAddress(const std::string &address);