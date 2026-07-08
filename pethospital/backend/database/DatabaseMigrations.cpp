#include "DatabaseMigrations.h"

#include "migrations/backfills/RelationBackfills.h"
#include "migrations/columns/ColumnMigrations.h"
#include "migrations/common/MigrationCommon.h"
#include "migrations/foreign_keys/ForeignKeyMigrations.h"
#include "../services/redis/RedisClient.h"
#include "../services/redis/redisLock/RedisLock.h"

#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <thread>

// 每张表一条 TableSpec —— 建表 DDL 原样保留（仅由字符串拼接改为 raw string），
// 表不存在时执行 createSql + afterCreate（种子/回填），已存在时执行 onExists（补列/补外键等增量迁移）。
// 新增表只需按依赖顺序追加一条 spec；不要修改已有表的 createSql，增量改动写进对应的
// migrations/columns / migrations/foreign_keys / migrations/backfills 模块。
namespace
{
    namespace Columns = DatabaseMigrations::Columns;
    namespace ForeignKeys = DatabaseMigrations::ForeignKeys;
    namespace Backfills = DatabaseMigrations::Backfills;
    namespace Common = DatabaseMigrations::Common;

    constexpr const char *kOperationRoleVarchar = "VARCHAR(64)";

    // 操作日志保存“当时职位名”的文本快照。仅当旧库仍是 ENUM 或长度不一致时才 ALTER，
    // 避免每次启动都跑 DDL 抢元数据锁。
    void alignOperationRoleVarchar(DatabaseManagerInterface &dbManager, mysqlx::Session &session,
                                   const std::string &table, const std::string &column)
    {
        const auto current = Common::getColumnType(dbManager, table, column);
        if (current && Common::normalizeSqlType(*current) == Common::normalizeSqlType(kOperationRoleVarchar))
        {
            std::cout << table << "." << column << " role snapshot column already up to date." << std::endl;
            return;
        }
        session.sql("ALTER TABLE " + table + " MODIFY COLUMN " + column + " " +
                    kOperationRoleVarchar + " NULL")
            .execute();
        std::cout << table << "." << column << " role snapshot column aligned." << std::endl;
    }

    struct TableSpec
    {
        const char *name;
        const char *createSql;
        // 建表成功后的一次性动作（种子数据 / 历史回填）；可为 nullptr。
        void (*afterCreate)(DatabaseManagerInterface &, mysqlx::Session &);
        // 表已存在时的增量迁移；可为 nullptr。
        void (*onExists)(DatabaseManagerInterface &, mysqlx::Session &);
    };

    void seedBranches(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql(R"SQL(INSERT INTO branches (name, system_key, is_system) VALUES
            ('总院', 'main', 1)
        )SQL")
            .execute();
    }

    void ensureDefaultBranch(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql(R"SQL(INSERT IGNORE INTO branches (name, system_key, is_system) VALUES
            ('总院', 'main', 1)
        )SQL")
            .execute();
    }

    void seedDepartments(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql(R"SQL(INSERT INTO departments (branch_id, name, system_key, sort_order, is_system)
            SELECT b.id, v.name, v.system_key, v.sort_order, 1
            FROM branches AS b
            JOIN (
                SELECT '管理部' AS name, 'management' AS system_key, 10 AS sort_order
                UNION ALL SELECT '财务部', 'finance', 20
                UNION ALL SELECT '人事部', 'personnel', 30
                UNION ALL SELECT '医疗部', 'medical', 40
                UNION ALL SELECT '仓储部', 'warehouse', 50
            ) AS v
            WHERE b.system_key = 'main'
        )SQL")
            .execute();
    }

    void seedPositions(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql(R"SQL(INSERT INTO positions (department_id, name, staff_kind, system_key, status)
            SELECT id, '总裁', 'management', 'president', 'published' FROM departments WHERE system_key = 'management'
            UNION ALL SELECT id, '副总裁', 'management', 'vice-president', 'published' FROM departments WHERE system_key = 'management'
            UNION ALL SELECT id, '部门经理', 'management', 'department-manager', 'published' FROM departments WHERE system_key = 'management'
            UNION ALL SELECT id, '超级管理员', 'management', 'super-admin', 'published' FROM departments WHERE system_key = 'management'
            UNION ALL SELECT id, '财务总监', 'finance', 'finance-director', 'published' FROM departments WHERE system_key = 'finance'
            UNION ALL SELECT id, '财务经理', 'finance', 'finance-manager', 'published' FROM departments WHERE system_key = 'finance'
            UNION ALL SELECT id, '人事经理', 'personnel', 'personnel-manager', 'published' FROM departments WHERE system_key = 'personnel'
            UNION ALL SELECT id, '医生', 'doctor', 'doctor', 'published' FROM departments WHERE system_key = 'medical'
            UNION ALL SELECT id, '护士', 'nurse', 'nurse', 'published' FROM departments WHERE system_key = 'medical'
            UNION ALL SELECT id, '仓库管理员', 'warehouse', 'warehouse-admin', 'published' FROM departments WHERE system_key = 'warehouse'
        )SQL")
            .execute();
    }

    void seedPositionPermissions(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql(R"SQL(INSERT INTO position_permissions (position_id, permission_key)
            SELECT p.id, v.permission_key
            FROM positions p
            JOIN (
                SELECT 'president' AS system_key, 'portal:boss' AS permission_key
                UNION ALL SELECT 'president', 'portal:user'
                UNION ALL SELECT 'president', 'portal:finance'
                UNION ALL SELECT 'president', 'portal:super-admin'
                UNION ALL SELECT 'president', 'portal:personnel'
                UNION ALL SELECT 'president', 'portal:medical'
                UNION ALL SELECT 'president', 'portal:warehouse'
                UNION ALL SELECT 'president', 'salary:read'
                UNION ALL SELECT 'president', 'salary:write'
                UNION ALL SELECT 'president', 'logs:read'
                UNION ALL SELECT 'president', 'medical-record:read'
                UNION ALL SELECT 'president', 'medical-record:write'
                UNION ALL SELECT 'president', 'doctor-work:write'
                UNION ALL SELECT 'president', 'user:delete'
                UNION ALL SELECT 'president', 'equity:read'
                UNION ALL SELECT 'president', 'equity:write'
                UNION ALL SELECT 'president', 'stock:read'
                UNION ALL SELECT 'president', 'stock:write'
                UNION ALL SELECT 'president', 'staff-role:write'
                UNION ALL SELECT 'president', 'scope:all'
                UNION ALL SELECT 'vice-president', 'portal:boss'
                UNION ALL SELECT 'vice-president', 'portal:user'
                UNION ALL SELECT 'vice-president', 'portal:finance'
                UNION ALL SELECT 'vice-president', 'portal:super-admin'
                UNION ALL SELECT 'vice-president', 'portal:personnel'
                UNION ALL SELECT 'vice-president', 'portal:medical'
                UNION ALL SELECT 'vice-president', 'portal:warehouse'
                UNION ALL SELECT 'vice-president', 'salary:read'
                UNION ALL SELECT 'vice-president', 'salary:write'
                UNION ALL SELECT 'vice-president', 'logs:read'
                UNION ALL SELECT 'vice-president', 'medical-record:read'
                UNION ALL SELECT 'vice-president', 'medical-record:write'
                UNION ALL SELECT 'vice-president', 'doctor-work:write'
                UNION ALL SELECT 'vice-president', 'user:delete'
                UNION ALL SELECT 'vice-president', 'equity:read'
                UNION ALL SELECT 'vice-president', 'equity:write'
                UNION ALL SELECT 'vice-president', 'stock:read'
                UNION ALL SELECT 'vice-president', 'stock:write'
                UNION ALL SELECT 'vice-president', 'staff-role:write'
                UNION ALL SELECT 'vice-president', 'scope:all'
                UNION ALL SELECT 'finance-director', 'portal:finance'
                UNION ALL SELECT 'finance-director', 'salary:read'
                UNION ALL SELECT 'finance-director', 'salary:write'
                UNION ALL SELECT 'finance-manager', 'portal:finance'
                UNION ALL SELECT 'finance-manager', 'salary:read'
                UNION ALL SELECT 'finance-manager', 'salary:write'
                UNION ALL SELECT 'department-manager', 'portal:super-admin'
                UNION ALL SELECT 'department-manager', 'logs:read'
                UNION ALL SELECT 'department-manager', 'medical-record:read'
                UNION ALL SELECT 'department-manager', 'doctor-work:write'
                UNION ALL SELECT 'department-manager', 'user:delete'
                UNION ALL SELECT 'super-admin', 'portal:super-admin'
                UNION ALL SELECT 'super-admin', 'logs:read'
                UNION ALL SELECT 'super-admin', 'medical-record:read'
                UNION ALL SELECT 'super-admin', 'doctor-work:write'
                UNION ALL SELECT 'super-admin', 'user:delete'
                UNION ALL SELECT 'personnel-manager', 'portal:personnel'
                UNION ALL SELECT 'personnel-manager', 'staff-role:write'
                UNION ALL SELECT 'doctor', 'portal:medical'
                UNION ALL SELECT 'doctor', 'medical-record:read'
                UNION ALL SELECT 'doctor', 'medical-record:write'
                UNION ALL SELECT 'doctor', 'scope:medical-assigned'
                UNION ALL SELECT 'nurse', 'portal:medical'
                UNION ALL SELECT 'nurse', 'medical-record:read'
                UNION ALL SELECT 'nurse', 'medical-record:write'
                UNION ALL SELECT 'nurse', 'scope:medical-assigned'
                UNION ALL SELECT 'warehouse-admin', 'portal:warehouse'
                UNION ALL SELECT 'warehouse-admin', 'stock:read'
                UNION ALL SELECT 'warehouse-admin', 'stock:write'
            ) v ON v.system_key = p.system_key
        )SQL")
            .execute();
    }

    void seedPermissionTemplates(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql(R"SQL(INSERT INTO permission_templates (name) VALUES
            ('Boss'),
            ('Finance'),
            ('SuperAdmin'),
            ('Personnel'),
            ('Medical'),
            ('Warehouse')
        )SQL")
            .execute();
    }

    void seedPermissionTemplateItems(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql(R"SQL(INSERT INTO permission_template_items (template_id, permission_key)
            SELECT t.id, v.permission_key
            FROM permission_templates t
            JOIN (
                SELECT 'Boss' AS template_name, 'portal:boss' AS permission_key
                UNION ALL SELECT 'Boss', 'portal:user'
                UNION ALL SELECT 'Boss', 'portal:finance'
                UNION ALL SELECT 'Boss', 'portal:super-admin'
                UNION ALL SELECT 'Boss', 'portal:personnel'
                UNION ALL SELECT 'Boss', 'portal:medical'
                UNION ALL SELECT 'Boss', 'portal:warehouse'
                UNION ALL SELECT 'Boss', 'salary:read'
                UNION ALL SELECT 'Boss', 'salary:write'
                UNION ALL SELECT 'Boss', 'logs:read'
                UNION ALL SELECT 'Boss', 'medical-record:read'
                UNION ALL SELECT 'Boss', 'medical-record:write'
                UNION ALL SELECT 'Boss', 'doctor-work:write'
                UNION ALL SELECT 'Boss', 'user:delete'
                UNION ALL SELECT 'Boss', 'equity:read'
                UNION ALL SELECT 'Boss', 'equity:write'
                UNION ALL SELECT 'Boss', 'stock:read'
                UNION ALL SELECT 'Boss', 'stock:write'
                UNION ALL SELECT 'Boss', 'staff-role:write'
                UNION ALL SELECT 'Boss', 'scope:all'
                UNION ALL SELECT 'Finance', 'portal:finance'
                UNION ALL SELECT 'Finance', 'salary:read'
                UNION ALL SELECT 'Finance', 'salary:write'
                UNION ALL SELECT 'SuperAdmin', 'portal:super-admin'
                UNION ALL SELECT 'SuperAdmin', 'logs:read'
                UNION ALL SELECT 'SuperAdmin', 'medical-record:read'
                UNION ALL SELECT 'SuperAdmin', 'doctor-work:write'
                UNION ALL SELECT 'SuperAdmin', 'user:delete'
                UNION ALL SELECT 'Personnel', 'portal:personnel'
                UNION ALL SELECT 'Personnel', 'staff-role:write'
                UNION ALL SELECT 'Medical', 'portal:medical'
                UNION ALL SELECT 'Medical', 'medical-record:read'
                UNION ALL SELECT 'Medical', 'medical-record:write'
                UNION ALL SELECT 'Medical', 'scope:medical-assigned'
                UNION ALL SELECT 'Warehouse', 'portal:warehouse'
                UNION ALL SELECT 'Warehouse', 'stock:read'
                UNION ALL SELECT 'Warehouse', 'stock:write'
            ) v ON v.template_name = t.name
        )SQL")
            .execute();
    }

    void migrateUserOperationsOrgScope(DatabaseManagerInterface &dbManager, mysqlx::Session &session)
    {
        Common::addColumnIfNotExists(dbManager, "user_operations", "operator_department_id", "INT NULL");
        if (!Common::foreignKeyExists(dbManager, "user_operations", "fk_user_operations_operator_department"))
        {
            session.sql("ALTER TABLE user_operations "
                        "ADD CONSTRAINT fk_user_operations_operator_department "
                        "FOREIGN KEY (operator_department_id) REFERENCES departments(id)")
                .execute();
        }
        Common::addIndexIfNotExists(dbManager, "user_operations", "idx_user_operations_operator_department", "operator_department_id");
    }

    void migrateDepartmentOrgColumns(DatabaseManagerInterface &dbManager, mysqlx::Session &session)
    {
        ensureDefaultBranch(dbManager, session);
        Common::addColumnIfNotExists(dbManager, "departments", "description", "VARCHAR(255) NOT NULL DEFAULT ''");
        Common::addColumnIfNotExists(dbManager, "departments", "branch_id", "INT NULL");
        session.sql("UPDATE departments "
                    "SET branch_id = (SELECT id FROM branches WHERE system_key = 'main' LIMIT 1) "
                    "WHERE branch_id IS NULL")
            .execute();
        session.sql("ALTER TABLE departments MODIFY COLUMN branch_id INT NOT NULL").execute();
        if (!Common::foreignKeyExists(dbManager, "departments", "fk_department_branch"))
        {
            session.sql("ALTER TABLE departments "
                        "ADD CONSTRAINT fk_department_branch "
                        "FOREIGN KEY (branch_id) REFERENCES branches(id)")
                .execute();
        }
    }

    void migratePositionDescriptionColumn(DatabaseManagerInterface &dbManager, mysqlx::Session &)
    {
        Common::addColumnIfNotExists(dbManager, "positions", "description", "VARCHAR(255) NOT NULL DEFAULT ''");
    }

    void ensureBootstrapSuperAdmin(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        mysqlx::Row userCountRow = session.sql("SELECT COUNT(*) FROM users WHERE is_deleted = 0").execute().fetchOne();
        if (userCountRow && !userCountRow[0].isNull() && userCountRow[0].get<int>() > 0)
        {
            return;
        }

        const char *password = std::getenv("PETMANAGER_BOOTSTRAP_ADMIN_PASSWORD");
        if (password == nullptr || std::string(password).empty())
        {
            std::cout << "Bootstrap super-admin skipped: set PETMANAGER_BOOTSTRAP_ADMIN_PASSWORD when initializing an empty users table." << std::endl;
            return;
        }

        const char *emailEnv = std::getenv("PETMANAGER_BOOTSTRAP_ADMIN_EMAIL");
        const std::string email = (emailEnv != nullptr && std::string(emailEnv).find('@') != std::string::npos)
                                      ? std::string(emailEnv)
                                      : "admin@petmanager.local";

        mysqlx::Row positionRow = session.sql("SELECT id FROM positions WHERE system_key = 'super-admin' LIMIT 1").execute().fetchOne();
        if (!positionRow || positionRow[0].isNull())
        {
            std::cout << "Bootstrap super-admin skipped: super-admin position is missing." << std::endl;
            return;
        }

        session.sql("INSERT INTO users (account_type, position_id, name, password, email) "
                    "VALUES ('staff', ?, '系统管理员', SHA2(?, 256), ?)")
            .bind(positionRow[0].get<int>(), std::string(password), email)
            .execute();
        std::cout << "Bootstrap super-admin user created from environment configuration." << std::endl;
    }

    // 建表顺序即外键依赖顺序：RBAC lookup → users → 其余子表。
    const TableSpec kTables[] = {
        {
            "branches",
            R"SQL(CREATE TABLE branches (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                name VARCHAR(64) NOT NULL UNIQUE,
                system_key VARCHAR(32) NULL UNIQUE,
                is_system TINYINT NOT NULL DEFAULT 0,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            seedBranches,
            ensureDefaultBranch,
        },
        {
            "departments",
            R"SQL(CREATE TABLE departments (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                branch_id INT NOT NULL,
                name VARCHAR(64) NOT NULL UNIQUE,
                description VARCHAR(255) NOT NULL DEFAULT '',
                system_key VARCHAR(32) NULL UNIQUE,
                sort_order INT NOT NULL DEFAULT 0,
                is_system TINYINT NOT NULL DEFAULT 0,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                CONSTRAINT fk_department_branch FOREIGN KEY (branch_id) REFERENCES branches(id)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            seedDepartments,
            migrateDepartmentOrgColumns,
        },
        {
            "positions",
            R"SQL(CREATE TABLE positions (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                department_id INT NOT NULL,
                name VARCHAR(64) NOT NULL,
                description VARCHAR(255) NOT NULL DEFAULT '',
                staff_kind ENUM('doctor','nurse','warehouse','finance','management','personnel','general_staff') NOT NULL DEFAULT 'general_staff',
                system_key VARCHAR(32) NULL UNIQUE,
                status ENUM('draft','published') NOT NULL DEFAULT 'published',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                CONSTRAINT fk_position_dept FOREIGN KEY (department_id) REFERENCES departments(id),
                UNIQUE KEY uq_position (department_id, name)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            seedPositions,
            migratePositionDescriptionColumn,
        },
        {
            "position_permissions",
            R"SQL(CREATE TABLE position_permissions (
                position_id INT NOT NULL,
                permission_key VARCHAR(64) NOT NULL,
                granted_by INT NULL,
                granted_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                PRIMARY KEY (position_id, permission_key),
                CONSTRAINT fk_pp_position FOREIGN KEY (position_id) REFERENCES positions(id) ON DELETE CASCADE,
                CONSTRAINT chk_position_permission_not_meta CHECK (permission_key <> 'rbac:manage')
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            seedPositionPermissions,
            nullptr,
        },
        {
            "permission_templates",
            R"SQL(CREATE TABLE permission_templates (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                name VARCHAR(64) NOT NULL UNIQUE
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            seedPermissionTemplates,
            nullptr,
        },
        {
            "permission_template_items",
            R"SQL(CREATE TABLE permission_template_items (
                template_id INT NOT NULL,
                permission_key VARCHAR(64) NOT NULL,
                PRIMARY KEY (template_id, permission_key),
                CONSTRAINT fk_pti_template FOREIGN KEY (template_id) REFERENCES permission_templates(id) ON DELETE CASCADE,
                CONSTRAINT chk_template_permission_not_meta CHECK (permission_key <> 'rbac:manage')
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            seedPermissionTemplateItems,
            nullptr,
        },
        {
            "salaryRecord",
            R"SQL(CREATE TABLE salaryRecord (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                salesCount DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '销售金额',
                costCount DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '成本金额',
                profitCount DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '利润金额',
                record_type ENUM('day', 'month') NOT NULL DEFAULT 'day',
                is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除',
                deleted_at DATETIME NULL COMMENT '软删除时间',
                deleted_by INT NULL COMMENT '执行删除的用户ID',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                INDEX idx_salaryRecord_is_deleted (is_deleted)
            ))SQL",
            nullptr,
            nullptr,
        },
        {
            "users",
            R"SQL(CREATE TABLE users (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                account_type ENUM('customer','staff') NOT NULL DEFAULT 'customer',
                position_id INT NULL,
                name VARCHAR(255) NOT NULL DEFAULT '',
                password VARCHAR(255) NOT NULL DEFAULT '',
                email VARCHAR(255) NOT NULL DEFAULT '',
                birthday DATE NULL DEFAULT NULL,
                head_image VARCHAR(255) NOT NULL DEFAULT '',
                user_specialty VARCHAR(255) NOT NULL DEFAULT '',
                user_introduction TEXT NOT NULL DEFAULT (''),
                user_level INT NOT NULL DEFAULT 0,
                funds DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '用户账户余额',
                is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除',
                deleted_at DATETIME NULL COMMENT '软删除时间',
                deleted_by INT NULL COMMENT '执行删除的用户ID',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_users_position FOREIGN KEY (position_id) REFERENCES positions(id),
                CONSTRAINT chk_account_position CHECK (
                    (account_type = 'customer' AND position_id IS NULL) OR
                    (account_type = 'staff' AND position_id IS NOT NULL)
                ),
                INDEX idx_users_name (name),
                INDEX idx_users_position_id (position_id),
                INDEX idx_users_account_type (account_type),
                INDEX idx_users_is_deleted (is_deleted)
            ))SQL",
            ensureBootstrapSuperAdmin,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                Columns::migrateUsers(dbManager);
                ForeignKeys::migrateUsers(dbManager);
                ensureBootstrapSuperAdmin(dbManager, *dbManager.getSession());
            },
        },
        {
            "user_scopes",
            R"SQL(CREATE TABLE user_scopes (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL,
                branch_id INT NULL,
                department_id INT NULL,
                granted_by INT NULL,
                granted_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                CONSTRAINT fk_user_scope_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                CONSTRAINT fk_user_scope_branch FOREIGN KEY (branch_id) REFERENCES branches(id),
                CONSTRAINT fk_user_scope_department FOREIGN KEY (department_id) REFERENCES departments(id),
                CONSTRAINT fk_user_scope_granted_by FOREIGN KEY (granted_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT chk_user_scope_level CHECK (branch_id IS NOT NULL OR department_id IS NOT NULL),
                UNIQUE KEY uq_user_scope_branch (user_id, branch_id),
                UNIQUE KEY uq_user_scope_department (user_id, department_id)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
        },
        {
            "address",
            R"SQL(CREATE TABLE address (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL COMMENT '地址所属用户ID',
                contact_name VARCHAR(80) NOT NULL DEFAULT '' COMMENT '联系人姓名',
                contact_phone VARCHAR(20) NOT NULL DEFAULT '' COMMENT '联系人手机号',
                country VARCHAR(80) NOT NULL DEFAULT '中国' COMMENT '国家或地区',
                province VARCHAR(80) NOT NULL DEFAULT '' COMMENT '省/直辖市/自治区',
                city VARCHAR(80) NOT NULL DEFAULT '' COMMENT '城市',
                district VARCHAR(80) NOT NULL DEFAULT '' COMMENT '区/县',
                detail_address VARCHAR(255) NOT NULL DEFAULT '' COMMENT '门牌号及详细地址',
                address_text VARCHAR(500) NOT NULL DEFAULT '' COMMENT '完整地址文本',
                postal_code VARCHAR(20) NOT NULL DEFAULT '' COMMENT '邮政编码，用户选填或后端根据地址库补全',
                address_tag ENUM('家', '公司', '医院', '学校', '其他') NOT NULL DEFAULT '家' COMMENT '地址标签',
                is_default TINYINT NOT NULL DEFAULT 0 COMMENT '是否默认地址',
                longitude DECIMAL(10, 7) NOT NULL DEFAULT 0.0000000 COMMENT '经度',
                latitude DECIMAL(10, 7) NOT NULL DEFAULT 0.0000000 COMMENT '纬度',
                geocode_source VARCHAR(50) NOT NULL DEFAULT '' COMMENT '地理编码来源',
                remarks VARCHAR(255) NOT NULL DEFAULT '' COMMENT '备注',
                is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除',
                deleted_at DATETIME NULL COMMENT '软删除时间',
                deleted_by INT NULL COMMENT '执行删除的用户ID',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_address_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                INDEX idx_address_user_default (user_id, is_default, is_deleted),
                INDEX idx_address_contact_phone (contact_phone),
                INDEX idx_address_region (province, city, district),
                INDEX idx_address_is_deleted (is_deleted)
            ))SQL",
            nullptr,
            nullptr,
        },
        {
            "salary",
            R"SQL(CREATE TABLE salary (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL,
                base_salary DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '基本工资',
                PA_Award DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '全勤奖',
                PB_Award DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '绩效奖金',
                total_salary DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '总工资',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_salary_userId FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                UNIQUE KEY uq_salary_user_id (user_id)
            ))SQL",
            nullptr,
            nullptr,
        },
        {
            "phones",
            R"SQL(CREATE TABLE phones (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL,
                phone VARCHAR(20) NOT NULL DEFAULT '',
                phone_lastfour VARCHAR(4) GENERATED ALWAYS AS (SUBSTRING(phone, -4)),
                CONSTRAINT fk_phones_users FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                INDEX idx_phone_exp (phone),
                INDEX idx_phone_lastfour (phone_lastfour)
            ))SQL",
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                Backfills::backfillPhones(dbManager);
            },
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                ForeignKeys::migratePhones(dbManager);
                Columns::migratePhones(dbManager);
                Backfills::backfillPhones(dbManager);
            },
        },
        {
            "pets",
            R"SQL(CREATE TABLE pets (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL,
                pet_name VARCHAR(255) NOT NULL DEFAULT '',
                pet_type VARCHAR(255) NOT NULL DEFAULT '',
                pet_age VARCHAR(255) NOT NULL DEFAULT '',
                pet_sex VARCHAR(255) NOT NULL DEFAULT '',
                pet_breed VARCHAR(255) NOT NULL DEFAULT '',
                pet_neutered VARCHAR(255) NOT NULL DEFAULT '',
                vaccine_status VARCHAR(255) NOT NULL DEFAULT '',
                preference TEXT NOT NULL DEFAULT (''),
                notes TEXT NOT NULL DEFAULT (''),
                is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除',
                deleted_at DATETIME NULL COMMENT '软删除时间',
                deleted_by INT NULL COMMENT '执行删除的用户ID',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_pets_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                INDEX idx_pets_is_deleted (is_deleted)
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                Columns::migratePets(dbManager);
                ForeignKeys::migratePets(dbManager);
            },
        },
        {
            "userSearch",
            R"SQL(CREATE TABLE userSearch (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL,
                search_text VARCHAR(255) NOT NULL DEFAULT '',
                is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除',
                deleted_at DATETIME NULL COMMENT '软删除时间',
                deleted_by INT NULL COMMENT '执行删除的用户ID',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_userSearch_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                UNIQUE KEY uk_userSearch_user_text (user_id, search_text),
                INDEX idx_userSearch_recent (user_id, is_deleted, updated_at)
            ))SQL",
            nullptr,
            nullptr,
        },
        {
            "monthlySalaryRecord",
            R"SQL(CREATE TABLE monthlySalaryRecord (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                salesCount DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '销售金额',
                costCount DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '成本金额',
                profitCount DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '利润金额',
                business_date DATE NOT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
            ))SQL",
            nullptr,
            nullptr,
        },
        {
            "stock",
            R"SQL(CREATE TABLE stock (
                id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
                share_type VARCHAR(255) NOT NULL DEFAULT '',
                holder VARCHAR(255) NOT NULL DEFAULT '',
                share BIGINT NOT NULL DEFAULT 0,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                UNIQUE KEY uq_stock_share_type_holder (share_type, holder)
            ))SQL",
            nullptr,
            nullptr,
        },
        {
            "warehouse",
            R"SQL(CREATE TABLE warehouse (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                item_name VARCHAR(255) NOT NULL DEFAULT '',
                item_type VARCHAR(255) NOT NULL DEFAULT '',
                item_productiondate DATE NULL DEFAULT NULL,
                item_expirationdate DATE NULL DEFAULT NULL,
                days_until_expire INT DEFAULT NULL,
                item_price DECIMAL(10, 2) NOT NULL DEFAULT 0.00,
                item_number INT NOT NULL DEFAULT 0,
                item_totalprice DECIMAL(18, 2) GENERATED ALWAYS AS (item_price * item_number) STORED NOT NULL,
                is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除',
                deleted_at DATETIME NULL COMMENT '软删除时间',
                deleted_by INT NULL COMMENT '执行删除的用户ID',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                INDEX idx_id_exp (id, days_until_expire),
                INDEX idx_exp (days_until_expire),
                INDEX idx_warehouse_is_deleted (is_deleted)
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                Columns::migrateWarehouse(dbManager);
            },
        },
        {
            "workTimes",
            R"SQL(CREATE TABLE workTimes (
                check_in_time_start TIME NOT NULL DEFAULT '00:00:00',
                check_in_time_end TIME NOT NULL DEFAULT '00:00:00',
                check_out_time_start TIME NOT NULL DEFAULT '00:00:00',
                check_out_time_end TIME NOT NULL DEFAULT '00:00:00'
            ))SQL",
            nullptr,
            nullptr,
        },
        {
            "onlineDoctors",
            R"SQL(CREATE TABLE onlineDoctors (
                doctor_id INT NOT NULL PRIMARY KEY,
                date DATE NOT NULL DEFAULT '1970-01-01',
                check_in_time TIME NOT NULL DEFAULT '00:00:00',
                check_out_time TIME NOT NULL DEFAULT '00:00:00',
                status ENUM('online', 'offline') NOT NULL DEFAULT 'offline',
                CONSTRAINT fk_doctor_user FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                ForeignKeys::migrateOnlineDoctors(dbManager);
            },
        },
        {
            // 有效预约唯一键 active_slot_key：仅当预约占位(status 非 cancelled/failed、未软删、有日期与时段)
            // 时取非 NULL 值，否则为 NULL(NULL 不参与唯一性)，从而实现"同医生同日同时段仅一笔有效预约"
            // 且取消/软删后可重订。用 VIRTUAL 生成列：避免 STORED 重建整表时与具名外键撞名(ERROR 1215)。
            "reservations",
            R"SQL(CREATE TABLE reservations (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL,
                doctor_id INT NOT NULL,
                pet_id INT NULL DEFAULT NULL,
                reservation_type VARCHAR(30) NOT NULL DEFAULT '',
                date DATE NULL DEFAULT NULL,
                time_slot VARCHAR(20) NULL DEFAULT NULL,
                status ENUM('scheduled', 'failed', 'cancelled', 'arrived') NOT NULL DEFAULT 'scheduled',
                is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除',
                deleted_at DATETIME NULL COMMENT '软删除时间',
                deleted_by INT NULL COMMENT '执行删除的用户ID',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                active_slot_key VARCHAR(80) GENERATED ALWAYS AS (CASE WHEN COALESCE(status,'scheduled') NOT IN ('cancelled','failed') AND is_deleted = 0 AND date IS NOT NULL AND time_slot IS NOT NULL THEN CONCAT(doctor_id, '|', date, '|', time_slot) ELSE NULL END) VIRTUAL,
                CONSTRAINT fk_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                CONSTRAINT fk_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE,
                CONSTRAINT fk_pet_id FOREIGN KEY (pet_id) REFERENCES pets(id) ON DELETE CASCADE,
                INDEX idx_user_deleted (user_id, is_deleted),
                INDEX idx_doctorId_date_slot (doctor_id, date, time_slot),
                INDEX idx_petId_date (pet_id, date),
                UNIQUE INDEX uq_active_slot (active_slot_key)
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                ForeignKeys::migrateReservations(dbManager);
                Columns::migrateReservations(dbManager);
            },
        },
        {
            "orders",
            R"SQL(CREATE TABLE orders (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                owner_id INT NOT NULL,
                pet_id INT NOT NULL,
                doctor_id INT NOT NULL,
                order_type VARCHAR(255) NOT NULL DEFAULT '',
                order_data VARCHAR(255) NOT NULL DEFAULT '',
                order_status ENUM('pending_payment', 'paid', 'cancelled', 'refunded', 'partial_refund') NOT NULL DEFAULT 'pending_payment',
                order_totalprice DECIMAL(18, 2) NOT NULL DEFAULT 0.00,
                is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除',
                deleted_at DATETIME NULL COMMENT '软删除时间',
                deleted_by INT NULL COMMENT '执行删除的用户ID',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_orders_owner_id FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE,
                CONSTRAINT fk_orders_pet_id FOREIGN KEY (pet_id) REFERENCES pets(id) ON DELETE CASCADE,
                CONSTRAINT fk_orders_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE,
                INDEX idx_orders_owner_deleted (owner_id, is_deleted),
                INDEX idx_orders_doctor_time (doctor_id, created_at),
                INDEX idx_petId_time (pet_id, created_at)
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                ForeignKeys::migrateOrders(dbManager);
                Columns::migrateOrders(dbManager);
            },
        },
        {
            "orderMedicines",
            R"SQL(CREATE TABLE orderMedicines (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                order_id INT NOT NULL,
                medicine_id INT NOT NULL,
                medicine_name VARCHAR(255) NOT NULL DEFAULT '',
                quantity INT NOT NULL DEFAULT 0,
                price DECIMAL(18, 2) NOT NULL DEFAULT 0.00,
                total_price DECIMAL(18, 2) NOT NULL DEFAULT 0.00,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                INDEX idx_order_id (order_id),
                INDEX idx_medicine_id (medicine_id),
                INDEX idx_order_medicine (order_id, medicine_id),
                CONSTRAINT fk_order_medicines_order_id FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE CASCADE
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                ForeignKeys::migrateOrderMedicines(dbManager);
            },
        },
        {
            "workTimeLogs",
            R"SQL(CREATE TABLE workTimeLogs (
                id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
                doctor_id INT NOT NULL,
                date DATE NOT NULL DEFAULT '1970-01-01',
                check_in_time TIME NOT NULL DEFAULT '00:00:00',
                check_out_time TIME NOT NULL DEFAULT '00:00:00',
                status ENUM('normal', 'late', 'early_leave', 'overtime', 'abnormal') NOT NULL DEFAULT 'normal',
                notes TEXT NOT NULL DEFAULT (''),
                is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除',
                deleted_at DATETIME NULL COMMENT '软删除时间',
                deleted_by INT NULL COMMENT '执行删除的用户ID',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_workLogs_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE,
                INDEX idx_user_deleted (doctor_id, is_deleted)
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                ForeignKeys::migrateWorkTimeRecords(dbManager);
            },
        },
        {
            "medicalQueueCounters",
            R"SQL(CREATE TABLE medicalQueueCounters (
                id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
                queue_date DATE NOT NULL COMMENT '队列日期',
                current_number INT NOT NULL DEFAULT 0 COMMENT '当天已分配的最大队列号',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                UNIQUE INDEX idx_medicalQueueCounters_queue_date (queue_date)
            ))SQL",
            nullptr,
            nullptr,
        },
        {
            "medicalQueues",
            R"SQL(CREATE TABLE medicalQueues (
                id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
                queue_date DATE NOT NULL COMMENT '队列日期',
                queue_number VARCHAR(32) NOT NULL COMMENT '队列号/序号',
                doctor_id INT NOT NULL,
                pet_id INT NOT NULL,
                owner_id INT NOT NULL,
                status ENUM('waiting', 'in_progress', 'skipped', 'cancelled', 'completed') NOT NULL DEFAULT 'waiting',
                source ENUM('appointment', 'walk_in', 'follow_up', 'emergency') NOT NULL DEFAULT 'walk_in',
                triage_level ENUM('normal', 'priority', 'urgent') NOT NULL DEFAULT 'normal',
                scheduled_at DATETIME NULL COMMENT '预约时间',
                arrived_at DATETIME NULL COMMENT '到院时间',
                is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_medicalQueues_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE,
                CONSTRAINT fk_medicalQueues_pet_id FOREIGN KEY (pet_id) REFERENCES pets(id) ON DELETE CASCADE,
                CONSTRAINT fk_medicalQueues_owner_id FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE,
                UNIQUE INDEX idx_medicalQueues_queue_date_number (queue_date, queue_number),
                INDEX idx_medicalQueues_doctor_status (doctor_id, status, is_deleted),
                INDEX idx_medicalQueues_doctor_date (doctor_id, queue_date, is_deleted),
                INDEX idx_medicalQueues_doctor_arrived (doctor_id, arrived_at),
                INDEX idx_medicalQueues_owner_id (owner_id),
                INDEX idx_medicalQueues_pet_id (pet_id)
            ))SQL",
            nullptr,
            nullptr,
        },
        {
            "system_operations",
            R"SQL(CREATE TABLE system_operations (
                id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
                category ENUM('系统类') NOT NULL DEFAULT '系统类',
                system_role VARCHAR(64) NULL,
                operator VARCHAR(255) NOT NULL DEFAULT '系统',
                module VARCHAR(255) NOT NULL DEFAULT '',
                action VARCHAR(100) NOT NULL DEFAULT '',
                result ENUM('成功', '警告', '失败') NOT NULL DEFAULT '成功',
                summary TEXT NOT NULL DEFAULT (''),
                details TEXT NOT NULL DEFAULT (''),
                source VARCHAR(255) NOT NULL DEFAULT 'system_operations',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                INDEX idx_time (created_at),
                INDEX idx_system_category (category),
                INDEX idx_system_result (result)
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &session)
            {
                alignOperationRoleVarchar(dbManager, session, "system_operations", "system_role");
            },
        },
        {
            "user_operations",
            R"SQL(CREATE TABLE user_operations (
                id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
                user_id INT NOT NULL,
                category ENUM('用户类') NOT NULL DEFAULT '用户类',
                user_role VARCHAR(64) NULL,
                operator_department_id INT NULL,
                operator VARCHAR(255) NOT NULL DEFAULT '',
                module VARCHAR(255) NOT NULL DEFAULT '',
                action VARCHAR(100) NOT NULL DEFAULT '',
                result ENUM('成功', '警告', '失败') NOT NULL DEFAULT '成功',
                summary TEXT NOT NULL DEFAULT (''),
                details TEXT NOT NULL DEFAULT (''),
                source VARCHAR(255) NOT NULL DEFAULT 'user_operations',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                INDEX idx_user_time (user_id, created_at),
                INDEX idx_user_category (category),
                INDEX idx_user_result (result),
                INDEX idx_user_operations_operator_department (operator_department_id),
                CONSTRAINT fk_user_operations_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                CONSTRAINT fk_user_operations_operator_department FOREIGN KEY (operator_department_id) REFERENCES departments(id)
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &session)
            {
                ForeignKeys::migrateUserOperations(dbManager);
                alignOperationRoleVarchar(dbManager, session, "user_operations", "user_role");
                migrateUserOperationsOrgScope(dbManager, session);
            },
        },
    };
}

namespace DatabaseMigrations
{
    // 迁移总入口。
    // 这里负责三件事：
    // 1. 扫描现有表并决定“创建”还是“增量迁移”
    // 2. 创建首次部署缺失的表
    // 3. 将补列、补外键、历史数据回填分别分发给对应模块
    void run(DatabaseManagerInterface &dbManager)
    {
        static bool tables_created = false;
        static std::mutex migration_mutex;
        std::lock_guard<std::mutex> lock(migration_mutex);

        if (tables_created)
        {
            std::cout << "Database migrations already applied in this process, skipping." << std::endl;
            return;
        }

        auto *session = dbManager.getSession();
        auto *schema = dbManager.getSchema();
        if (!session || !schema)
        {
            std::cerr << "Database schema is null, cannot run migrations." << std::endl;
            return;
        }

        // 跨实例启动迁移锁：多实例同时启动时避免并发 DDL / 回填。
        // 抢到锁的实例先迁移，其余实例阻塞等待；迁移本身幂等，等到后各自再跑也只是重复的存在性检查。
        // Redis 不可用时退回原行为（各实例直接迁移），绝不阻断启动。
        const std::string kMigrationLockKey = "migration:startup:lock";
        const int lockTtlSeconds = 600; // 安全网：迁移正常仅数秒，远小于此，避免持锁实例崩溃后死锁
        RedisLockGuard migrationLock;   // token 安全；析构自动释放（仅当仍是自己持有）
        if (RedisClient::instance().enabled())
        {
            const auto waitDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(600);
            while (true)
            {
                RedisLock::Result res = RedisLock::tryAcquire(kMigrationLockKey, lockTtlSeconds);
                if (res.outcome == RedisLock::Outcome::Unavailable)
                {
                    // Redis 出错：降级，直接迁移，不空等到超时。
                    std::cout << "Migration startup lock unavailable (Redis error); proceeding." << std::endl;
                    break;
                }
                if (res.outcome == RedisLock::Outcome::Acquired)
                {
                    migrationLock = std::move(res.guard);
                    break;
                }
                // Contended：别的实例正在迁移，等待到超时再放行。
                if (std::chrono::steady_clock::now() >= waitDeadline)
                {
                    std::cout << "Migration startup lock wait timed out; proceeding without it." << std::endl;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        try
        {
            session->sql("SET SESSION lock_wait_timeout = 10").execute();
        }
        catch (const std::exception &e)
        {
            std::cout << "Failed to set lock_wait_timeout: " << e.what() << std::endl;
        }

        std::set<std::string> existing;
        for (const auto &table : schema->getTables())
        {
            existing.insert(table.getName());
        }

        for (const TableSpec &spec : kTables)
        {
            if (existing.count(spec.name))
            {
                std::cout << spec.name << " table already exists." << std::endl;
                if (spec.onExists)
                {
                    spec.onExists(dbManager, *session);
                }
            }
            else
            {
                std::cout << spec.name << " table does not exist. Creating..." << std::endl;
                session->sql(spec.createSql).execute();
                if (spec.afterCreate)
                {
                    spec.afterCreate(dbManager, *session);
                }
                std::cout << spec.name << " table created successfully." << std::endl;
            }
        }

        tables_created = true;
    }
}
