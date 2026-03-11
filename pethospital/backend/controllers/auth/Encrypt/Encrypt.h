#pragma once
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <vector>

std::string sha256_hash(const std::string &input);                                        // 计算SHA256哈希值
std::string hash_password(const std::string &password);                                   // 生成带盐的密码哈希
bool verify_password_hash(const std::string &password, const std::string &stored_hash);   // 校验密码哈希
bool password_hash_needs_upgrade(const std::string &stored_hash);                         // 判断是否需要升级旧哈希
