#pragma once
#include <iostream>
#include <string>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

// 声明辅助函数
std::string url_safe_base64_encode(const std::string &data);
std::string read_config_value(const std::string &filename, const std::string &key);
std::string get_jwt_secret();
std::string url_safe_base64_encode(const std::string &data);
std::string generate_SHA256_jwt(const std::string &email);

// 计算SHA256哈希值
std::string sha256_hash(const std::string &input);