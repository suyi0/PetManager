#pragma once
#include <iostream>
#include <string>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

std::string sha256_hash(const std::string &input);                                        // 计算SHA256哈希值
