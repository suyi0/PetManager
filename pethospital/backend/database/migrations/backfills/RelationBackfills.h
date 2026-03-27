#pragma once

#include "DatabaseManagerInterface.h"

namespace DatabaseMigrations::Backfills
{
// 全局关系表历史数据回填开关。
// 受 DB_AUTO_BACKFILL_RELATION_TABLES 控制。
bool shouldAutoBackfillRelationTables();

// phones 表历史数据回填开关。
// 受 DB_AUTO_BACKFILL_PHONES_FROM_USERS 控制，也会被全局开关覆盖。
bool shouldAutoBackfillPhonesFromUsers();

// phones 表关系回填入口。
// 根据 users.id -> phones.user_id、users.phone -> phones.phone 的映射补历史数据。
void backfillPhones(DatabaseManagerInterface &database_manager);
}
