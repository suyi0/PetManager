#pragma once

#include "DatabaseManagerInterface.h"

#include <cstdint>
#include <optional>
#include <string>

namespace DatabaseMigrations::Common
{
// 外键迁移规则描述。
// 用统一结构承载约束名、子表列、父表列，避免在调用点到处手写 SQL 片段。
struct ForeignKeySpec
{
    std::string table_name;
    std::string constraint_name;
    std::string column_name;
    std::string referenced_table;
    std::string referenced_column;
    std::string on_delete_action = "CASCADE";
};

// 环境变量工具：清理包裹引号并统一解析布尔值。
std::string trimQuotes(std::string value);
bool isTruthyEnv(const char *value);

// 动态 SQL 安全工具：只允许安全标识符进入表名、列名、索引名拼接。
bool isSafeIdentifier(const std::string &identifier);
std::string quoteIdentifier(const std::string &identifier);

// 列类型标准化：用于比较外键两端的真实数据库类型是否兼容。
std::string normalizeSqlType(std::string type);

// 元数据查询：用于增量迁移前判断对象是否存在。
bool columnExists(DatabaseManagerInterface &database_manager, const std::string &table_name, const std::string &column_name);
std::optional<std::string> getColumnType(DatabaseManagerInterface &database_manager, const std::string &table_name, const std::string &column_name);
bool foreignKeyExists(DatabaseManagerInterface &database_manager, const std::string &table_name, const std::string &constraint_name);
bool indexExists(DatabaseManagerInterface &database_manager, const std::string &table_name, const std::string &index_name);

// 历史数据校验：补外键前先统计孤儿数据，避免 ALTER TABLE 直接失败。
std::int64_t countOrphanRows(DatabaseManagerInterface &database_manager, const ForeignKeySpec &spec);

// DDL 辅助：为已有旧库补列/补索引，且保证幂等。
bool addColumnIfNotExists(
    DatabaseManagerInterface &database_manager,
    const std::string &table_name,
    const std::string &column_name,
    const std::string &column_definition);

bool addIndexIfNotExists(
    DatabaseManagerInterface &database_manager,
    const std::string &table_name,
    const std::string &index_name,
    const std::string &column_name);
}
