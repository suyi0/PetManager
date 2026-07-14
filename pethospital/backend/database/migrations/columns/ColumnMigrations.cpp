#include "migrations/columns/ColumnMigrations.h"

#include "migrations/common/MigrationCommon.h"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

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
    auto *session = database_manager.getSession();
    if (!session)
    {
        return;
    }

    // --- payrollPeriod columns (nullable first; backfill later) ---
    Common::addColumnIfNotExists(database_manager, "payrollPeriod", "review_note",
                                 "VARCHAR(1000) NOT NULL DEFAULT ''");
    Common::addColumnIfNotExists(database_manager, "payrollPeriod", "submitted_by", "INT NULL");
    Common::addColumnIfNotExists(database_manager, "payrollPeriod", "submitted_at", "DATETIME NULL");
    Common::addColumnIfNotExists(database_manager, "payrollPeriod", "supervisor_reviewed_by", "INT NULL");
    Common::addColumnIfNotExists(database_manager, "payrollPeriod", "supervisor_reviewed_at", "DATETIME NULL");
    Common::addColumnIfNotExists(database_manager, "payrollPeriod", "supervisor_decision",
                                 "ENUM('approve','return') NULL");
    Common::addColumnIfNotExists(database_manager, "payrollPeriod", "supervisor_note",
                                 "VARCHAR(1000) NOT NULL DEFAULT ''");
    Common::addColumnIfNotExists(database_manager, "payrollPeriod", "revision_of_period_id", "INT NULL");
    Common::addColumnIfNotExists(database_manager, "payrollPeriod", "row_version",
                                 "INT NOT NULL DEFAULT 1");

    // --- salary columns ---
    Common::addColumnIfNotExists(database_manager, "salary", "first_reviewed_by", "INT NULL");
    Common::addColumnIfNotExists(database_manager, "salary", "first_reviewed_at", "DATETIME NULL");
    Common::addColumnIfNotExists(database_manager, "salary", "review_note",
                                 "VARCHAR(1000) NOT NULL DEFAULT ''");

    try
    {
        // Expand period status enum to include submitted_for_supervisor / correction_required.
        // Always rewrite to the full target set so restarts stay idempotent.
        const auto periodStatusType = Common::getColumnType(database_manager, "payrollPeriod", "status");
        const std::string periodStatusTarget =
            "ENUM('calculating','first_review','submitted_for_supervisor','second_review',"
            "'correction_required','locked','archived') NOT NULL DEFAULT 'calculating'";
        if (!periodStatusType ||
            Common::normalizeSqlType(*periodStatusType).find("submitted_for_supervisor") == std::string::npos)
        {
            session->sql("ALTER TABLE payrollPeriod MODIFY COLUMN status " + periodStatusTarget).execute();
            std::cout << "payrollPeriod.status enum expanded for supervisor review." << std::endl;
        }

        // Keep legacy second_reviewed/locked readable; new flow only writes pending/first_reviewed/returned.
        const auto salaryStatusType = Common::getColumnType(database_manager, "salary", "review_status");
        const std::string salaryStatusTarget =
            "ENUM('pending','first_reviewed','returned','second_reviewed','locked') NOT NULL DEFAULT 'pending'";
        if (!salaryStatusType ||
            Common::normalizeSqlType(*salaryStatusType).find("returned") == std::string::npos)
        {
            session->sql("ALTER TABLE salary MODIFY COLUMN review_status " + salaryStatusTarget).execute();
            std::cout << "salary.review_status enum expanded with returned." << std::endl;
        }

        // Legacy second_review was "submitted but no trusted supervisor decision" — map to waiting state.
        // Do not invent supervisor_* fields for these rows.
        session->sql(
                   "UPDATE payrollPeriod SET status='submitted_for_supervisor' "
                   "WHERE status='second_review' "
                   "AND (supervisor_decision IS NULL OR supervisor_decision='')")
            .execute();

        // Legacy second_reviewed rows represented completion of the old second-review step.
        // After the period is moved to the new supervisor queue, they must re-enter the
        // first-review state so a supervisor return can be corrected and resubmitted.
        session->sql(
                   "UPDATE salary s JOIN payrollPeriod p ON p.id=s.payroll_period_id "
                   "SET s.review_status='first_reviewed' "
                   "WHERE p.status='submitted_for_supervisor' "
                   "AND (p.supervisor_decision IS NULL OR p.supervisor_decision='') "
                   "AND s.review_status='second_reviewed'")
            .execute();

        // Old reviewed_by/reviewed_at meant "who submitted for second review", not supervisor approval.
        session->sql(
                   "UPDATE payrollPeriod SET submitted_by=COALESCE(submitted_by, reviewed_by), "
                   "submitted_at=COALESCE(submitted_at, reviewed_at) "
                   "WHERE reviewed_by IS NOT NULL AND submitted_by IS NULL")
            .execute();

        session->sql(
                   "CREATE TABLE IF NOT EXISTS payrollPeriodAuditEvent ("
                   "id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,"
                   "period_id INT NOT NULL,"
                   "version_no INT NOT NULL DEFAULT 1,"
                   "before_row_version INT NULL,"
                   "after_row_version INT NULL,"
                   "action VARCHAR(64) NOT NULL,"
                   "decision VARCHAR(32) NULL,"
                   "operator_id INT NULL,"
                   "operator_department_id INT NULL,"
                   "before_status VARCHAR(64) NULL,"
                   "after_status VARCHAR(64) NULL,"
                   "note VARCHAR(1000) NOT NULL DEFAULT '',"
                   "request_id VARCHAR(64) NOT NULL DEFAULT '',"
                   "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                   "INDEX idx_payroll_audit_period_created (period_id, created_at),"
                   "INDEX idx_payroll_audit_operator (operator_id, created_at),"
                   "CONSTRAINT fk_payroll_audit_period FOREIGN KEY (period_id) "
                   "REFERENCES payrollPeriod(id) ON DELETE CASCADE,"
                   "CONSTRAINT fk_payroll_audit_operator FOREIGN KEY (operator_id) "
                   "REFERENCES users(id) ON DELETE SET NULL"
                   ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
            .execute();
    }
    catch (const std::exception &e)
    {
        std::cerr << "payroll supervisor-review migration failed: " << e.what() << std::endl;
    }
}

void migrateOrders(DatabaseManagerInterface &database_manager)
{
    auto *session = database_manager.getSession();

    // 必需列（与旧列清理开关无关，任何已部署库都必须补齐）：
    // orders.department_id 快照开单医生所属部门，财务按部门/分院归集营收。运行时下单/财务首页 SQL
    // 已强依赖此列，若延后到 legacy 开关才补列会导致「未知列」运行时错误。
    try
    {
        const bool departmentColumnExisted = Common::columnExists(database_manager, "orders", "department_id");
        Common::addColumnIfNotExists(database_manager, "orders", "department_id", "INT NULL");
        Common::addIndexIfNotExists(database_manager, "orders", "idx_orders_department", "department_id");
        if (!departmentColumnExisted)
        {
            // 首次加列后一次性回填历史订单，按开单医生当前所属部门推断；无法推断的保持 NULL（仅 scope:all 可见）。
            // 用 columnExists 预判确保只回填一次，避免每次启动全表扫描。
            session->sql("UPDATE orders AS o "
                         "JOIN users AS u ON u.id = o.doctor_id AND u.is_deleted = 0 "
                         "JOIN positions AS p ON p.id = u.position_id "
                         "SET o.department_id = p.department_id "
                         "WHERE o.department_id IS NULL AND p.department_id IS NOT NULL")
                .execute();
            std::cout << "orders.department_id added and backfilled from doctor's current department." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "orders.department_id migration failed: " << e.what() << std::endl;
    }

    if (!shouldAutoMigrateLegacyColumns())
    {
        std::cout << "Skipping orders legacy column migration. Set DB_AUTO_MIGRATE_LEGACY_COLUMNS=true to enable." << std::endl;
        return;
    }

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
