#include "migrations/columns/ColumnMigrations.h"

#include "migrations/common/MigrationCommon.h"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>

namespace DatabaseMigrations::Columns
{
bool shouldAutoMigrateLegacyColumns()
{
    return Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_LEGACY_COLUMNS"));
}

void migrateUsers(DatabaseManagerInterface &database_manager)
{
    Common::addIndexIfNotExists(database_manager, "users", "idx_users_name", "name");

    // 考勤模块：旧库补 attendance_no 列 + 唯一键（新库建表已含，全部幂等）。
    Common::addColumnIfNotExists(database_manager, "users", "attendance_no", "VARCHAR(32) NULL");
    if (!Common::indexExists(database_manager, "users", "uq_users_attendance_no"))
    {
        try
        {
            database_manager.getSession()
                ->sql("ALTER TABLE users ADD UNIQUE KEY uq_users_attendance_no (attendance_no)")
                .execute();
        }
        catch (const std::exception &e)
        {
            std::cerr << "users.uq_users_attendance_no add failed: " << e.what() << std::endl;
        }
    }
    // 设计 §3：设备协议只认 attendance_no，v1 用 users.id 字符串初始化存量员工，
    // 否则设备打卡匹配不到人、closeDay 全员跳过。
    try
    {
        database_manager.getSession()
            ->sql("UPDATE users SET attendance_no = CAST(id AS CHAR) "
                  "WHERE account_type = 'staff' AND (attendance_no IS NULL OR attendance_no = '')")
            .execute();
    }
    catch (const std::exception &e)
    {
        std::cerr << "users.attendance_no backfill failed: " << e.what() << std::endl;
    }
}

void migratePhones(DatabaseManagerInterface &database_manager)
{
    if (!Common::columnExists(database_manager, "phones", "phone_lastfour"))
    {
        if (Common::addColumnIfNotExists(
                database_manager,
                "phones",
                "phone_lastfour",
                "VARCHAR(4) GENERATED ALWAYS AS (SUBSTRING(phone, -4))"))
        {
            std::cout << "phones.phone_lastfour generated column added for legacy migration." << std::endl;
        }
        return;
    }

    std::cout << "phones.phone_lastfour already exists." << std::endl;
}

void migratePets(DatabaseManagerInterface &database_manager)
{
    Common::addColumnIfNotExists(database_manager, "pets", "pet_breed", "VARCHAR(255) NOT NULL DEFAULT ''");
    Common::addColumnIfNotExists(database_manager, "pets", "pet_neutered", "VARCHAR(255) NOT NULL DEFAULT ''");
    Common::addColumnIfNotExists(database_manager, "pets", "vaccine_status", "VARCHAR(255) NOT NULL DEFAULT ''");
    Common::addColumnIfNotExists(database_manager, "pets", "preference", "TEXT NOT NULL DEFAULT ('')");
    Common::addColumnIfNotExists(database_manager, "pets", "notes", "TEXT NOT NULL DEFAULT ('')");
}

void migrateWarehouse(DatabaseManagerInterface &database_manager)
{
    if (!shouldAutoMigrateLegacyColumns())
    {
        std::cout << "Skipping warehouse legacy column migration. Set DB_AUTO_MIGRATE_LEGACY_COLUMNS=true to enable." << std::endl;
        return;
    }

    auto *session = database_manager.getSession();
    try
    {
        if (!Common::columnExists(database_manager, "warehouse", "item_totalprice"))
        {
            if (Common::addColumnIfNotExists(
                    database_manager,
                    "warehouse",
                    "item_totalprice",
                    "DECIMAL(18, 2) GENERATED ALWAYS AS (item_price * item_number) STORED NOT NULL"))
            {
                std::cout << "warehouse.item_totalprice generated column added for legacy migration." << std::endl;
            }
        }
        else
        {
            session->sql("ALTER TABLE warehouse "
                         "MODIFY COLUMN item_totalprice DECIMAL(18, 2) "
                         "GENERATED ALWAYS AS (item_price * item_number) STORED NOT NULL")
                .execute();
            std::cout << "warehouse.item_totalprice migrated to generated column." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "warehouse.item_totalprice migration skipped: " << e.what() << std::endl;
    }
}

void migrateReservations(DatabaseManagerInterface &database_manager)
{
    // 与 CREATE TABLE 中的定义保持一致；必须用 VIRTUAL，STORED 会触发整表重建并与具名外键撞名(ERROR 1215)。
    Common::addColumnIfNotExists(
        database_manager,
        "reservations",
        "active_slot_key",
        "VARCHAR(80) GENERATED ALWAYS AS ("
        "CASE WHEN COALESCE(status,'scheduled') NOT IN ('cancelled','failed') "
        "AND is_deleted = 0 AND date IS NOT NULL AND time_slot IS NOT NULL "
        "THEN CONCAT(doctor_id, '|', date, '|', time_slot) ELSE NULL END) VIRTUAL");

    try
    {
        if (Common::indexExists(database_manager, "reservations", "uq_active_slot"))
        {
            std::cout << "Index 'uq_active_slot' already exists on table 'reservations'" << std::endl;
            return;
        }

        auto *session = database_manager.getSession();

        // 存量数据若已有同医生同日同时段的多笔有效预约，直接建唯一索引会失败；
        // 先探测并明确报出来，交由人工处理，不让迁移在启动期报错中断。
        auto dupResult = session->sql(
                                    "SELECT COUNT(*) FROM ("
                                    "SELECT doctor_id FROM reservations "
                                    "WHERE COALESCE(status,'scheduled') NOT IN ('cancelled','failed') "
                                    "AND is_deleted = 0 AND date IS NOT NULL AND time_slot IS NOT NULL "
                                    "GROUP BY doctor_id, date, time_slot HAVING COUNT(*) > 1"
                                    ") AS dup")
                             .execute();
        auto dupRow = dupResult.fetchOne();
        const std::int64_t duplicateSlots = dupRow ? dupRow[0].get<std::int64_t>() : 0;
        if (duplicateSlots > 0)
        {
            std::cerr << "Skipping unique index 'uq_active_slot': found " << duplicateSlots
                      << " duplicated active reservation slot(s). Resolve duplicates manually, then restart." << std::endl;
            return;
        }

        session->sql("ALTER TABLE reservations ADD UNIQUE INDEX uq_active_slot (active_slot_key)").execute();
        std::cout << "Added unique index 'uq_active_slot' to table 'reservations'" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "reservations.uq_active_slot migration skipped: " << e.what() << std::endl;
    }
}

void migratePayrollPeriod(DatabaseManagerInterface &database_manager)
{
    Common::addColumnIfNotExists(
        database_manager,
        "payrollPeriod",
        "review_note",
        "VARCHAR(1000) NOT NULL DEFAULT ''");
}

void migrateOrders(DatabaseManagerInterface &database_manager)
{
    if (!shouldAutoMigrateLegacyColumns())
    {
        std::cout << "Skipping orders legacy column migration. Set DB_AUTO_MIGRATE_LEGACY_COLUMNS=true to enable." << std::endl;
        return;
    }

    auto *session = database_manager.getSession();
    try
    {
        if (!Common::columnExists(database_manager, "orders", "order_data"))
        {
            if (Common::addColumnIfNotExists(database_manager, "orders", "order_data", "VARCHAR(255) NOT NULL DEFAULT ''"))
            {
                std::cout << "orders.order_data added for legacy migration." << std::endl;
            }
        }
        else
        {
            std::cout << "orders.order_data is already exists, migration skipped: column." << std::endl;
        }

        if (Common::columnExists(database_manager, "orders", "order_date"))
        {
            session->sql("UPDATE orders "
                         "SET order_data = CAST(order_date AS CHAR) "
                         "WHERE order_date IS NOT NULL "
                         "AND (order_data IS NULL OR order_data = '')")
                .execute();
            std::cout << "orders.order_date values copied into order_data." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "orders.order_data migration skipped: " << e.what() << std::endl;
    }
}
}
