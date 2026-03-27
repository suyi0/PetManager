#include "migrations/columns/ColumnMigrations.h"

#include "migrations/common/MigrationCommon.h"

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
                    "DECIMAL(18, 2) GENERATED ALWAYS AS (item_price * item_number) STORED"))
            {
                std::cout << "warehouse.item_totalprice generated column added for legacy migration." << std::endl;
            }
        }
        else
        {
            session->sql("ALTER TABLE warehouse "
                         "MODIFY COLUMN item_totalprice DECIMAL(18, 2) "
                         "GENERATED ALWAYS AS (item_price * item_number) STORED")
                .execute();
            std::cout << "warehouse.item_totalprice migrated to generated column." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "warehouse.item_totalprice migration skipped: " << e.what() << std::endl;
    }
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
            if (Common::addColumnIfNotExists(database_manager, "orders", "order_data", "VARCHAR(255)"))
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
