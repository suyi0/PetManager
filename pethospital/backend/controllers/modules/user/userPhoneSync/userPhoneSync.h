#pragma once

#include "DatabaseManagerInterface.h"

#include <string>

namespace UserPhoneSync
{
// 将用户手机号写入 phones 表。
// 适用于新增用户和更新手机号这两类业务写路径。
bool upsertUserPhone(DatabaseManagerInterface &dbManager, int user_id, const std::string &phone);
bool upsertUserPhone(mysqlx::Session &session, int user_id, const std::string &phone);
}
