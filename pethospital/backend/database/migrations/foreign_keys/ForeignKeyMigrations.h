#pragma once

#include "DatabaseManagerInterface.h"

namespace DatabaseMigrations::ForeignKeys
{
// 全局外键自动迁移开关。
// 受 DB_AUTO_MIGRATE_FOREIGN_KEYS 控制，开启后所有分表外键迁移函数都会视为启用。
bool shouldAutoMigrateForeignKeys();

// 分表外键迁移开关。
// 这些函数允许你只开启某一张表的外键修复，而不是一次性放开全部历史库变更。
bool shouldAutoMigrateUsersForeignKeys();
bool shouldAutoMigratePhonesForeignKeys();
bool shouldAutoMigrateOnlineDoctorsForeignKeys();
bool shouldAutoMigrateReservationsForeignKeys();
bool shouldAutoMigratePetsForeignKeys();
bool shouldAutoMigrateOrdersForeignKeys();
bool shouldAutoMigrateOrderMedicinesForeignKeys();
bool shouldAutoMigrateWorkTimeRecordsForeignKeys();
bool shouldAutoMigrateUserOperationsForeignKeys();

// 分表外键迁移入口。
// 每个函数只负责一张表自身的历史外键补齐逻辑，便于后续独立维护。
void migrateUsers(DatabaseManagerInterface &database_manager);
void migratePhones(DatabaseManagerInterface &database_manager);
void migrateOnlineDoctors(DatabaseManagerInterface &database_manager);
void migrateReservations(DatabaseManagerInterface &database_manager);
void migratePets(DatabaseManagerInterface &database_manager);
void migrateOrders(DatabaseManagerInterface &database_manager);
void migrateOrderMedicines(DatabaseManagerInterface &database_manager);
void migrateWorkTimeRecords(DatabaseManagerInterface &database_manager);
void migrateUserOperations(DatabaseManagerInterface &database_manager);
}
