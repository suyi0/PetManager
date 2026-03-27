#include "migrations/backfills/RelationBackfills.h"

#include "migrations/common/MigrationCommon.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace DatabaseMigrations::Backfills
{
namespace
{
bool backfillPhonesFromUsers(DatabaseManagerInterface &database_manager)
{
    try
    {
        database_manager.getSession()
            ->sql("INSERT INTO phones (user_id, phone) "
                  "SELECT u.id, u.phone "
                  "FROM users AS u "
                  "WHERE u.phone IS NOT NULL "
                  "AND TRIM(u.phone) <> '' "
                  "AND NOT EXISTS ("
                  "    SELECT 1 FROM phones AS p "
                  "    WHERE p.user_id = u.id AND p.phone = u.phone"
                  ")")
            .execute();

        std::cout << "Backfilled legacy phone records from users into phones." << std::endl;
        return true;
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "Error backfilling phones from users: " << e.what() << std::endl;
        return false;
    }
}

void backfillRelationIfEnabled(
    DatabaseManagerInterface &database_manager,
    const std::string &backfill_name,
    bool enabled,
    const std::string &env_hint,
    bool (*backfill_fn)(DatabaseManagerInterface &))
{
    if (!enabled)
    {
        std::cout << "Skipping relation backfill for " << backfill_name
                  << ". Set " << env_hint << "=true to enable." << std::endl;
        return;
    }

    if (backfill_fn(database_manager))
    {
        std::cout << "Relation backfill completed for " << backfill_name << "." << std::endl;
    }
}
}

bool shouldAutoBackfillRelationTables()
{
    return Common::isTruthyEnv(std::getenv("DB_AUTO_BACKFILL_RELATION_TABLES"));
}

bool shouldAutoBackfillPhonesFromUsers()
{
    return shouldAutoBackfillRelationTables() ||
           Common::isTruthyEnv(std::getenv("DB_AUTO_BACKFILL_PHONES_FROM_USERS"));
}

void backfillPhones(DatabaseManagerInterface &database_manager)
{
    backfillRelationIfEnabled(
        database_manager,
        "phones <- users.phone",
        shouldAutoBackfillPhonesFromUsers(),
        "DB_AUTO_BACKFILL_PHONES_FROM_USERS or DB_AUTO_BACKFILL_RELATION_TABLES",
        backfillPhonesFromUsers);
}
}
