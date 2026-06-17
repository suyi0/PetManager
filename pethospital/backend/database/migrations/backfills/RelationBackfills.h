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
// 仅在旧库仍存在 users.phone 时，将历史手机号搬到 phones 表。
void backfillPhones(DatabaseManagerInterface &database_manager);
}
