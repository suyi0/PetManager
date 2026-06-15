#include "migrations/foreign_keys/ForeignKeyMigrations.h"

#include "migrations/common/MigrationCommon.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace DatabaseMigrations::ForeignKeys
{
namespace
{
bool addForeignKeyIfNotExists(DatabaseManagerInterface &database_manager, const Common::ForeignKeySpec &spec)
{
    try
    {
        if (!Common::isSafeIdentifier(spec.table_name) ||
            !Common::isSafeIdentifier(spec.constraint_name) ||
            !Common::isSafeIdentifier(spec.column_name) ||
            !Common::isSafeIdentifier(spec.referenced_table) ||
            !Common::isSafeIdentifier(spec.referenced_column))
        {
            throw std::runtime_error("Unsafe identifier in addForeignKeyIfNotExists");
        }

        if (Common::foreignKeyExists(database_manager, spec.table_name, spec.constraint_name))
        {
            std::cout << "Foreign key '" << spec.constraint_name << "' already exists in table '" << spec.table_name << "'" << std::endl;
            return true;
        }

        if (!Common::columnExists(database_manager, spec.table_name, spec.column_name))
        {
            std::cerr << "Cannot add foreign key '" << spec.constraint_name << "': column '" << spec.column_name
                      << "' does not exist in table '" << spec.table_name << "'" << std::endl;
            return false;
        }

        if (!Common::columnExists(database_manager, spec.referenced_table, spec.referenced_column))
        {
            std::cerr << "Cannot add foreign key '" << spec.constraint_name << "': referenced column '"
                      << spec.referenced_table << "." << spec.referenced_column << "' does not exist" << std::endl;
            return false;
        }

        const auto child_type = Common::getColumnType(database_manager, spec.table_name, spec.column_name);
        const auto parent_type = Common::getColumnType(database_manager, spec.referenced_table, spec.referenced_column);
        if (!child_type || !parent_type)
        {
            std::cerr << "Cannot add foreign key '" << spec.constraint_name
                      << "': failed to inspect column types" << std::endl;
            return false;
        }

        if (Common::normalizeSqlType(*child_type) != Common::normalizeSqlType(*parent_type))
        {
            std::cerr << "Cannot add foreign key '" << spec.constraint_name << "': type mismatch between "
                      << spec.table_name << "." << spec.column_name << " (" << *child_type << ") and "
                      << spec.referenced_table << "." << spec.referenced_column << " (" << *parent_type << ")" << std::endl;
            return false;
        }

        const std::int64_t orphan_rows = Common::countOrphanRows(database_manager, spec);
        if (orphan_rows > 0)
        {
            std::cerr << "Cannot add foreign key '" << spec.constraint_name << "': found "
                      << orphan_rows << " orphan row(s) in '" << spec.table_name << "'. "
                      << "Clean those rows before enabling DB_AUTO_MIGRATE_FOREIGN_KEYS." << std::endl;
            return false;
        }

        const std::string sql =
            "ALTER TABLE " + Common::quoteIdentifier(spec.table_name) +
            " ADD CONSTRAINT " + Common::quoteIdentifier(spec.constraint_name) +
            " FOREIGN KEY (" + Common::quoteIdentifier(spec.column_name) + ") REFERENCES " +
            Common::quoteIdentifier(spec.referenced_table) + "(" + Common::quoteIdentifier(spec.referenced_column) + ")" +
            " ON DELETE " + spec.on_delete_action;

        database_manager.getSession()->sql(sql).execute();
        std::cout << "Added foreign key '" << spec.constraint_name << "' to table '" << spec.table_name << "'" << std::endl;
        return true;
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "Error adding foreign key '" << spec.constraint_name << "' to table '"
                  << spec.table_name << "': " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error preparing foreign key '" << spec.constraint_name << "' on table '"
                  << spec.table_name << "': " << e.what() << std::endl;
        return false;
    }
}

void migrateForeignKeyIfEnabled(
    DatabaseManagerInterface &database_manager,
    const Common::ForeignKeySpec &spec,
    bool enabled,
    const std::string &env_hint)
{
    if (!enabled)
    {
        std::cout << "Skipping foreign key migration for "
                  << spec.table_name << "." << spec.constraint_name
                  << ". Set " << env_hint << "=true to enable." << std::endl;
        return;
    }

    if (Common::foreignKeyExists(database_manager, spec.table_name, spec.constraint_name))
    {
        std::cout << spec.table_name << "." << spec.constraint_name
                  << " already exists, migration skipped." << std::endl;
        return;
    }

    if (addForeignKeyIfNotExists(database_manager, spec))
    {
        std::cout << spec.table_name << "." << spec.constraint_name
                  << " added for legacy migration." << std::endl;
    }
}

void addCompositeIndexIfNotExists(
    DatabaseManagerInterface &database_manager,
    const std::string &table_name,
    const std::string &index_name,
    const std::vector<std::string> &column_names)
{
    try
    {
        if (!Common::isSafeIdentifier(table_name) || !Common::isSafeIdentifier(index_name) || column_names.empty())
        {
            throw std::runtime_error("Unsafe identifier in addCompositeIndexIfNotExists");
        }

        if (Common::indexExists(database_manager, table_name, index_name))
        {
            std::cout << "Index '" << index_name << "' already exists on table '" << table_name << "'" << std::endl;
            return;
        }

        std::string columns;
        for (const auto &column_name : column_names)
        {
            if (!Common::isSafeIdentifier(column_name))
            {
                throw std::runtime_error("Unsafe column identifier in addCompositeIndexIfNotExists");
            }
            if (!Common::columnExists(database_manager, table_name, column_name))
            {
                std::cout << "Skip index '" << index_name << "': column '" << column_name
                          << "' does not exist in table '" << table_name << "'" << std::endl;
                return;
            }
            if (!columns.empty())
            {
                columns += ", ";
            }
            columns += Common::quoteIdentifier(column_name);
        }

        const std::string sql =
            "CREATE INDEX " + Common::quoteIdentifier(index_name) +
            " ON " + Common::quoteIdentifier(table_name) + " (" + columns + ")";
        database_manager.getSession()->sql(sql).execute();
        std::cout << "Added index '" << index_name << "' to table '" << table_name << "'" << std::endl;
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "Error adding index '" << index_name << "' to table '"
                  << table_name << "': " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error preparing index '" << index_name << "' on table '"
                  << table_name << "': " << e.what() << std::endl;
    }
}
}

bool shouldAutoMigrateForeignKeys()
{
    return Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_FOREIGN_KEYS"));
}

bool shouldAutoMigrateUsersForeignKeys()
{
    return shouldAutoMigrateForeignKeys() ||
           Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_USERS_FOREIGN_KEYS"));
}

bool shouldAutoMigratePhonesForeignKeys()
{
    return shouldAutoMigrateForeignKeys() ||
           Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_PHONES_FOREIGN_KEYS"));
}

bool shouldAutoMigrateOnlineDoctorsForeignKeys()
{
    return shouldAutoMigrateForeignKeys() ||
           Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_ONLINEDOCTORS_FOREIGN_KEYS"));
}

bool shouldAutoMigrateReservationsForeignKeys()
{
    return shouldAutoMigrateForeignKeys() ||
           Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_RESERVATIONS_FOREIGN_KEYS"));
}

bool shouldAutoMigratePetsForeignKeys()
{
    return shouldAutoMigrateForeignKeys() ||
           Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_PETS_FOREIGN_KEYS"));
}

bool shouldAutoMigrateOrdersForeignKeys()
{
    return shouldAutoMigrateForeignKeys() ||
           Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_ORDERS_FOREIGN_KEYS"));
}

bool shouldAutoMigrateOrderMedicinesForeignKeys()
{
    return shouldAutoMigrateForeignKeys() ||
           Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_ORDERMEDICINES_FOREIGN_KEYS"));
}

bool shouldAutoMigrateWorkTimeRecordsForeignKeys()
{
    return shouldAutoMigrateForeignKeys() ||
           Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_WORKTIMERECORDS_FOREIGN_KEYS"));
}

bool shouldAutoMigrateUserOperationsForeignKeys()
{
    return shouldAutoMigrateForeignKeys() ||
           Common::isTruthyEnv(std::getenv("DB_AUTO_MIGRATE_USER_OPERATIONS_FOREIGN_KEYS"));
}

void migrateUsers(DatabaseManagerInterface &database_manager)
{
    migrateForeignKeyIfEnabled(
        database_manager,
        {"users", "fk_user_type", "type_id", "types", "id", "CASCADE"},
        shouldAutoMigrateUsersForeignKeys(),
        "DB_AUTO_MIGRATE_USERS_FOREIGN_KEYS");
}

void migratePhones(DatabaseManagerInterface &database_manager)
{
    migrateForeignKeyIfEnabled(
        database_manager,
        {"phones", "fk_phones_users", "user_id", "users", "id", "CASCADE"},
        shouldAutoMigratePhonesForeignKeys(),
        "DB_AUTO_MIGRATE_PHONES_FOREIGN_KEYS");
}

void migrateOnlineDoctors(DatabaseManagerInterface &database_manager)
{
    migrateForeignKeyIfEnabled(
        database_manager,
        {"onlineDoctors", "fk_doctor_user", "doctor_id", "users", "id", "CASCADE"},
        shouldAutoMigrateOnlineDoctorsForeignKeys(),
        "DB_AUTO_MIGRATE_ONLINEDOCTORS_FOREIGN_KEYS");
}

void migrateReservations(DatabaseManagerInterface &database_manager)
{
    Common::addColumnIfNotExists(database_manager, "reservations", "pet_id", "INT NOT NULL");
    addCompositeIndexIfNotExists(database_manager, "reservations", "idx_userId_creationTime", {"user_id", "created_at"});
    addCompositeIndexIfNotExists(database_manager, "reservations", "idx_userId_date", {"user_id", "date"});
    addCompositeIndexIfNotExists(database_manager, "reservations", "idx_doctorId_date_slot", {"doctor_id", "date", "time_slot"});
    addCompositeIndexIfNotExists(database_manager, "reservations", "idx_petId_date", {"pet_id", "date"});

    migrateForeignKeyIfEnabled(
        database_manager,
        {"reservations", "fk_user_id", "user_id", "users", "id", "CASCADE"},
        shouldAutoMigrateReservationsForeignKeys(),
        "DB_AUTO_MIGRATE_RESERVATIONS_FOREIGN_KEYS");
    migrateForeignKeyIfEnabled(
        database_manager,
        {"reservations", "fk_doctor_id", "doctor_id", "users", "id", "CASCADE"},
        shouldAutoMigrateReservationsForeignKeys(),
        "DB_AUTO_MIGRATE_RESERVATIONS_FOREIGN_KEYS");
    migrateForeignKeyIfEnabled(
        database_manager,
        {"reservations", "fk_pet_id", "pet_id", "pets", "id", "CASCADE"},
        shouldAutoMigrateReservationsForeignKeys(),
        "DB_AUTO_MIGRATE_RESERVATIONS_FOREIGN_KEYS");
}

void migratePets(DatabaseManagerInterface &database_manager)
{
    migrateForeignKeyIfEnabled(
        database_manager,
        {"pets", "fk_pets_user_id", "user_id", "users", "id", "CASCADE"},
        shouldAutoMigratePetsForeignKeys(),
        "DB_AUTO_MIGRATE_PETS_FOREIGN_KEYS");
}

void migrateOrders(DatabaseManagerInterface &database_manager)
{
    migrateForeignKeyIfEnabled(
        database_manager,
        {"orders", "fk_orders_pet_id", "pet_id", "pets", "id", "CASCADE"},
        shouldAutoMigrateOrdersForeignKeys(),
        "DB_AUTO_MIGRATE_ORDERS_FOREIGN_KEYS");
    migrateForeignKeyIfEnabled(
        database_manager,
        {"orders", "fk_orders_doctor_id", "doctor_id", "users", "id", "CASCADE"},
        shouldAutoMigrateOrdersForeignKeys(),
        "DB_AUTO_MIGRATE_ORDERS_FOREIGN_KEYS");
}

void migrateOrderMedicines(DatabaseManagerInterface &database_manager)
{
    migrateForeignKeyIfEnabled(
        database_manager,
        {"orderMedicines", "fk_order_id", "order_id", "orders", "id", "CASCADE"},
        shouldAutoMigrateOrderMedicinesForeignKeys(),
        "DB_AUTO_MIGRATE_ORDERMEDICINES_FOREIGN_KEYS");
    migrateForeignKeyIfEnabled(
        database_manager,
        {"orderMedicines", "fk_medicine_id", "medicine_id", "warehouse", "id", "CASCADE"},
        shouldAutoMigrateOrderMedicinesForeignKeys(),
        "DB_AUTO_MIGRATE_ORDERMEDICINES_FOREIGN_KEYS");
}

void migrateWorkTimeRecords(DatabaseManagerInterface &database_manager)
{
    migrateForeignKeyIfEnabled(
        database_manager,
        {"workTimeRecords", "fk_worktime_doctor_id", "doctor_id", "users", "id", "CASCADE"},
        shouldAutoMigrateWorkTimeRecordsForeignKeys(),
        "DB_AUTO_MIGRATE_WORKTIMERECORDS_FOREIGN_KEYS");
}

void migrateUserOperations(DatabaseManagerInterface &database_manager)
{
    migrateForeignKeyIfEnabled(
        database_manager,
        {"user_operations", "fk_user_operations_user_id", "user_id", "users", "id", "CASCADE"},
        shouldAutoMigrateUserOperationsForeignKeys(),
        "DB_AUTO_MIGRATE_USER_OPERATIONS_FOREIGN_KEYS");
}
}
