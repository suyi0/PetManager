#pragma once

#include "DatabaseManagerInterface.h"

namespace DatabaseMigrations::Columns
{
// 是否允许执行历史列兼容迁移。
// 受 DB_AUTO_MIGRATE_LEGACY_COLUMNS 控制。
bool shouldAutoMigrateLegacyColumns();

// users 表的列级迁移。
// 当前主要负责补索引等非约束类结构。
void migrateUsers(DatabaseManagerInterface &database_manager);

// phones 表的列级迁移。
// 当前负责补 phone_lastfour 生成列。
void migratePhones(DatabaseManagerInterface &database_manager);

// pets 表的列级迁移。
// 当前负责补用户端宠物档案扩展字段。
void migratePets(DatabaseManagerInterface &database_manager);

// warehouse 表的列级迁移。
// 当前负责把旧版 item_totalprice 迁成生成列。
void migrateWarehouse(DatabaseManagerInterface &database_manager);

// orders 表的列级迁移。
// 当前负责兼容旧字段 order_date -> order_data。
void migrateOrders(DatabaseManagerInterface &database_manager);
}
