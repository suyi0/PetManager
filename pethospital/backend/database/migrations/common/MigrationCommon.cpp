#include "migrations/common/MigrationCommon.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace DatabaseMigrations::Common
{
std::string trimQuotes(std::string value)
{
    if (value.size() >= 2)
    {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\''))
        {
            return value.substr(1, value.size() - 2);
        }
    }

    return value;
}

bool isTruthyEnv(const char *value)
{
    if (!value)
    {
        return false;
    }

    std::string normalized = trimQuotes(std::string(value));
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch)
                   { return static_cast<char>(std::toupper(ch)); });
    return normalized == "1" || normalized == "TRUE" || normalized == "YES" || normalized == "ON";
}

bool isSafeIdentifier(const std::string &identifier)
{
    if (identifier.empty())
    {
        return false;
    }

    return std::all_of(identifier.begin(), identifier.end(), [](unsigned char ch)
                       { return std::isalnum(ch) || ch == '_'; });
}

std::string quoteIdentifier(const std::string &identifier)
{
    if (!isSafeIdentifier(identifier))
    {
        throw std::runtime_error("Unsafe SQL identifier: " + identifier);
    }

    return "`" + identifier + "`";
}

std::string normalizeSqlType(std::string type)
{
    std::transform(type.begin(), type.end(), type.begin(), [](unsigned char ch)
                   { return static_cast<char>(std::tolower(ch)); });
    type.erase(std::remove_if(type.begin(), type.end(), [](unsigned char ch)
                              { return std::isspace(ch); }),
               type.end());
    return type;
}

bool columnExists(DatabaseManagerInterface &database_manager, const std::string &table_name, const std::string &column_name)
{
    auto result = database_manager.getSession()->sql(
                                                   "SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS "
                                                   "WHERE TABLE_SCHEMA = DATABASE() "
                                                   "AND TABLE_NAME = ? "
                                                   "AND COLUMN_NAME = ?")
                      .bind(table_name, column_name)
                      .execute();

    auto row = result.fetchOne();
    return row && row[0].get<int>() > 0;
}

std::optional<std::string> getColumnType(DatabaseManagerInterface &database_manager, const std::string &table_name, const std::string &column_name)
{
    auto result = database_manager.getSession()->sql(
                                                   "SELECT COLUMN_TYPE FROM INFORMATION_SCHEMA.COLUMNS "
                                                   "WHERE TABLE_SCHEMA = DATABASE() "
                                                   "AND TABLE_NAME = ? "
                                                   "AND COLUMN_NAME = ?")
                      .bind(table_name, column_name)
                      .execute();

    auto row = result.fetchOne();
    if (!row || row[0].isNull())
    {
        return std::nullopt;
    }

    return row[0].get<std::string>();
}

bool foreignKeyExists(DatabaseManagerInterface &database_manager, const std::string &table_name, const std::string &constraint_name)
{
    auto result = database_manager.getSession()->sql(
                                                   "SELECT COUNT(*) FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS "
                                                   "WHERE CONSTRAINT_SCHEMA = DATABASE() "
                                                   "AND TABLE_NAME = ? "
                                                   "AND CONSTRAINT_NAME = ? "
                                                   "AND CONSTRAINT_TYPE = 'FOREIGN KEY'")
                      .bind(table_name, constraint_name)
                      .execute();

    auto row = result.fetchOne();
    return row && row[0].get<int>() > 0;
}

bool indexExists(DatabaseManagerInterface &database_manager, const std::string &table_name, const std::string &index_name)
{
    auto result = database_manager.getSession()->sql(
                                                   "SELECT COUNT(*) FROM INFORMATION_SCHEMA.STATISTICS "
                                                   "WHERE TABLE_SCHEMA = DATABASE() "
                                                   "AND TABLE_NAME = ? "
                                                   "AND INDEX_NAME = ?")
                      .bind(table_name, index_name)
                      .execute();

    auto row = result.fetchOne();
    return row && row[0].get<int>() > 0;
}

std::int64_t countOrphanRows(DatabaseManagerInterface &database_manager, const ForeignKeySpec &spec)
{
    const std::string sql =
        "SELECT COUNT(*) "
        "FROM " +
        quoteIdentifier(spec.table_name) + " AS child "
                                           "LEFT JOIN " +
        quoteIdentifier(spec.referenced_table) + " AS parent "
                                                 "ON child." +
        quoteIdentifier(spec.column_name) + " = parent." + quoteIdentifier(spec.referenced_column) + " "
                                                                                                     "WHERE child." +
        quoteIdentifier(spec.column_name) + " IS NOT NULL "
                                            "AND parent." +
        quoteIdentifier(spec.referenced_column) + " IS NULL";

    auto result = database_manager.getSession()->sql(sql).execute();
    auto row = result.fetchOne();
    return row ? row[0].get<std::int64_t>() : 0;
}

bool addColumnIfNotExists(
    DatabaseManagerInterface &database_manager,
    const std::string &table_name,
    const std::string &column_name,
    const std::string &column_definition)
{
    try
    {
        if (!isSafeIdentifier(table_name) || !isSafeIdentifier(column_name))
        {
            throw std::runtime_error("Unsafe identifier in addColumnIfNotExists");
        }

        if (columnExists(database_manager, table_name, column_name))
        {
            std::cout << "Column '" << column_name << "' already exists in table '" << table_name << "'" << std::endl;
            return true;
        }

        const std::string sql =
            "ALTER TABLE " + quoteIdentifier(table_name) +
            " ADD COLUMN " + quoteIdentifier(column_name) + " " + column_definition;
        database_manager.getSession()->sql(sql).execute();
        std::cout << "Added column '" << column_name << "' to table '" << table_name << "'" << std::endl;
        return true;
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "Error adding column '" << column_name << "' to table '" << table_name << "': " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error preparing column migration for '" << table_name << "." << column_name
                  << "': " << e.what() << std::endl;
        return false;
    }
}

bool addIndexIfNotExists(
    DatabaseManagerInterface &database_manager,
    const std::string &table_name,
    const std::string &index_name,
    const std::string &column_name)
{
    try
    {
        if (!isSafeIdentifier(table_name) || !isSafeIdentifier(index_name) || !isSafeIdentifier(column_name))
        {
            throw std::runtime_error("Unsafe identifier in addIndexIfNotExists");
        }

        if (indexExists(database_manager, table_name, index_name))
        {
            std::cout << "Index '" << index_name << "' already exists on table '" << table_name << "'" << std::endl;
            return true;
        }

        const std::string sql =
            "CREATE INDEX " + quoteIdentifier(index_name) +
            " ON " + quoteIdentifier(table_name) + " (" + quoteIdentifier(column_name) + ")";
        database_manager.getSession()->sql(sql).execute();
        std::cout << "Added index '" << index_name << "' to table '" << table_name << "'" << std::endl;
        return true;
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "Error adding index '" << index_name << "' to table '" << table_name << "': " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error preparing index migration for '" << table_name << "." << index_name
                  << "': " << e.what() << std::endl;
        return false;
    }
}
}
