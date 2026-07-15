#include "DatabaseMigrations.h"

#include "migrations/backfills/RelationBackfills.h"
#include "migrations/columns/ColumnMigrations.h"
#include "migrations/common/MigrationCommon.h"
#include "migrations/foreign_keys/ForeignKeyMigrations.h"
#include "../controllers/auth/encrypt/encrypt.h"
#include "../services/redis/RedisClient.h"
#include "../services/redis/redisLock/RedisLock.h"
#include "../utils/permissions/Permissions.h"

#include <openssl/rand.h>

#include <array>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <set>
#include <string>
#include <thread>
#include <vector>

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

    std::string normalizeTableName(std::string name)
    {
        for (char &character : name)
        {
            character = static_cast<char>(
                std::tolower(static_cast<unsigned char>(character)));
        }
        return name;
    }

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
        session.sql(R"SQL(INSERT INTO positions (department_id, name, staff_kind, system_key, status, assignment_policy)
            SELECT id, '总裁', 'management', 'president', 'published', 'approval_required' FROM departments WHERE system_key = 'management'
            UNION ALL SELECT id, '副总裁', 'management', 'vice-president', 'published', 'approval_required' FROM departments WHERE system_key = 'management'
            UNION ALL SELECT id, '部门经理', 'management', 'department-manager', 'published', 'approval_required' FROM departments WHERE system_key = 'management'
            UNION ALL SELECT id, '超级管理员', 'management', 'super-admin', 'published', 'super_admin_only' FROM departments WHERE system_key = 'management'
            UNION ALL SELECT id, '财务总监', 'finance', 'finance-director', 'published', 'approval_required' FROM departments WHERE system_key = 'finance'
            UNION ALL SELECT id, '财务经理', 'finance', 'finance-manager', 'published', 'approval_required' FROM departments WHERE system_key = 'finance'
            UNION ALL SELECT id, '人事经理', 'personnel', 'personnel-manager', 'published', 'approval_required' FROM departments WHERE system_key = 'personnel'
            UNION ALL SELECT id, '医生', 'doctor', 'doctor', 'published', 'personnel_direct' FROM departments WHERE system_key = 'medical'
            UNION ALL SELECT id, '护士', 'nurse', 'nurse', 'published', 'personnel_direct' FROM departments WHERE system_key = 'medical'
            UNION ALL SELECT id, '仓库管理员', 'warehouse', 'warehouse-admin', 'published', 'personnel_direct' FROM departments WHERE system_key = 'warehouse'
        )SQL")
            .execute();
    }

    // 考勤权限增量 seed：seedPositionPermissions 只在建表时跑，早于考勤功能建库的实例
    // 永远拿不到 attendance:* —— 管理端整套考勤接口全员 403。仅当库里完全没有考勤
    // 权限时装设计默认值；一旦有任何一条（含管理员手动配置），不再干预，避免和动态
    // RBAC 的运行时回收打架。
    void seedAttendancePermissionsIfAbsent(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql(R"SQL(INSERT IGNORE INTO position_permissions (position_id, permission_key)
            SELECT p.id, v.permission_key
            FROM positions p
            JOIN (
                SELECT 'president' AS system_key, 'attendance:read' AS permission_key
                UNION ALL SELECT 'president', 'attendance:manage'
                UNION ALL SELECT 'vice-president', 'attendance:read'
                UNION ALL SELECT 'vice-president', 'attendance:manage'
                UNION ALL SELECT 'super-admin', 'attendance:read'
                UNION ALL SELECT 'personnel-manager', 'attendance:read'
                UNION ALL SELECT 'personnel-manager', 'attendance:manage'
                UNION ALL SELECT 'Boss', 'attendance:read'
                UNION ALL SELECT 'Boss', 'attendance:manage'
                UNION ALL SELECT 'SuperAdmin', 'attendance:read'
                UNION ALL SELECT 'Personnel', 'attendance:read'
                UNION ALL SELECT 'Personnel', 'attendance:manage'
            ) v ON v.system_key = p.system_key
            WHERE NOT EXISTS (
                SELECT 1 FROM position_permissions existing
                WHERE existing.permission_key IN ('attendance:read', 'attendance:manage')
            ))SQL")
            .execute();
    }

    void seedMedicalDocumentPermissionsIfAbsent(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql(R"SQL(INSERT IGNORE INTO position_permissions (position_id, permission_key)
            SELECT p.id, v.permission_key
            FROM positions p
            JOIN (
                SELECT 'president' AS system_key, 'medical-record:finalize' AS permission_key
                UNION ALL SELECT 'president', 'medical-record:print'
                UNION ALL SELECT 'president', 'medical-record:amend'
                UNION ALL SELECT 'president', 'medical-record:void'
                UNION ALL SELECT 'president', 'report-template:read'
                UNION ALL SELECT 'president', 'report-template:manage'
                UNION ALL SELECT 'president', 'report-template:publish'
                UNION ALL SELECT 'vice-president', 'medical-record:finalize'
                UNION ALL SELECT 'vice-president', 'medical-record:print'
                UNION ALL SELECT 'vice-president', 'medical-record:amend'
                UNION ALL SELECT 'vice-president', 'medical-record:void'
                UNION ALL SELECT 'vice-president', 'report-template:read'
                UNION ALL SELECT 'vice-president', 'report-template:manage'
                UNION ALL SELECT 'vice-president', 'report-template:publish'
                UNION ALL SELECT 'super-admin', 'medical-record:print'
                UNION ALL SELECT 'super-admin', 'report-template:read'
                UNION ALL SELECT 'super-admin', 'report-template:manage'
                UNION ALL SELECT 'super-admin', 'report-template:publish'
                UNION ALL SELECT 'doctor', 'medical-record:finalize'
                UNION ALL SELECT 'doctor', 'medical-record:print'
                UNION ALL SELECT 'doctor', 'medical-record:amend'
                UNION ALL SELECT 'doctor', 'medical-record:void'
            ) v ON v.system_key = p.system_key
            WHERE NOT EXISTS (
                SELECT 1 FROM position_permissions existing
                WHERE existing.position_id = p.id AND existing.permission_key = v.permission_key
            ))SQL")
            .execute();
    }

    void seedEmploymentPermissionKeysIfAbsent(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        // v6 seed 纪律：
        // - 管理职位（president/vice-president/department-manager）：仅 portal:boss + 两个审批权
        // - Personnel 新权限：仅人事域且当前持 staff-role:write 的职位（含 personnel-manager）
        // - Finance：system_key finance-* 仅 salary-profile:activate
        // - SuperAdmin：不默认薪酬批准/财务激活
        session.sql(R"SQL(INSERT IGNORE INTO position_permissions (position_id, permission_key)
            SELECT p.id, v.permission_key
            FROM positions p
            JOIN (
                SELECT 'president' AS system_key, 'portal:boss' AS permission_key
                UNION ALL SELECT 'president', 'employment-assignment:approve'
                UNION ALL SELECT 'president', 'compensation:approve'
                UNION ALL SELECT 'vice-president', 'portal:boss'
                UNION ALL SELECT 'vice-president', 'employment-assignment:approve'
                UNION ALL SELECT 'vice-president', 'compensation:approve'
                UNION ALL SELECT 'department-manager', 'portal:boss'
                UNION ALL SELECT 'department-manager', 'employment-assignment:approve'
                UNION ALL SELECT 'department-manager', 'compensation:approve'
                UNION ALL SELECT 'finance-director', 'salary-profile:activate'
                UNION ALL SELECT 'finance-manager', 'salary-profile:activate'
            ) v ON v.system_key = p.system_key
            WHERE NOT EXISTS (
                SELECT 1 FROM position_permissions existing
                WHERE existing.position_id = p.id AND existing.permission_key = v.permission_key
            ))SQL")
            .execute();

        // 人事域 + 已持 staff-role:write → 幂等插入 employment/compensation 人事键
        session.sql(R"SQL(INSERT IGNORE INTO position_permissions (position_id, permission_key)
            SELECT p.id, v.permission_key
            FROM positions p
            LEFT JOIN departments d ON d.id = p.department_id
            JOIN position_permissions srw
              ON srw.position_id = p.id AND srw.permission_key = 'staff-role:write'
            JOIN (
                SELECT 'employment:read' AS permission_key
                UNION ALL SELECT 'employment:onboard'
                UNION ALL SELECT 'employment:assign'
                UNION ALL SELECT 'employment:regularize'
                UNION ALL SELECT 'employment:offboard'
                UNION ALL SELECT 'compensation:propose'
                UNION ALL SELECT 'compensation:reassign-case'
            ) v
            WHERE (p.staff_kind = 'personnel' OR COALESCE(d.business_domain, '') = 'personnel')
              AND NOT EXISTS (
                SELECT 1 FROM position_permissions existing
                WHERE existing.position_id = p.id AND existing.permission_key = v.permission_key
              ))SQL")
            .execute();

        session.sql(R"SQL(INSERT IGNORE INTO permission_template_items (template_id, permission_key)
            SELECT t.id, v.permission_key
            FROM permission_templates t
            JOIN (
                SELECT 'Personnel' AS template_name, 'employment:read' AS permission_key
                UNION ALL SELECT 'Personnel', 'employment:onboard'
                UNION ALL SELECT 'Personnel', 'employment:assign'
                UNION ALL SELECT 'Personnel', 'employment:regularize'
                UNION ALL SELECT 'Personnel', 'employment:offboard'
                UNION ALL SELECT 'Personnel', 'compensation:propose'
                UNION ALL SELECT 'Personnel', 'compensation:reassign-case'
                UNION ALL SELECT 'Boss', 'portal:boss'
                UNION ALL SELECT 'Boss', 'employment-assignment:approve'
                UNION ALL SELECT 'Boss', 'compensation:approve'
                UNION ALL SELECT 'Finance', 'salary-profile:activate'
            ) v ON v.template_name = t.name
            WHERE NOT EXISTS (
                SELECT 1 FROM permission_template_items existing
                WHERE existing.template_id = t.id AND existing.permission_key = v.permission_key
            ))SQL")
            .execute();
    }

    void seedIncrementalPositionPermissions(DatabaseManagerInterface &dbManager, mysqlx::Session &session)
    {
        seedAttendancePermissionsIfAbsent(dbManager, session);
        seedMedicalDocumentPermissionsIfAbsent(dbManager, session);
        // 主管复审权限拆分：
        // - 财务负责人：初审 + 提交主管（不默认给 lock，避免提交人天然可锁定）
        // - 管理层：主管复审 + 锁定（不默认给 submit-review，避免同人复审）
        // 旧 salary:review 保留为财务初审语义；已授予的 lock 不在此删除。
        session.sql(R"SQL(INSERT IGNORE INTO position_permissions (position_id, permission_key)
            SELECT p.id, v.permission_key
            FROM positions p
            JOIN (
                SELECT 'president' AS system_key, 'salary:review' AS permission_key
                UNION ALL SELECT 'president', 'salary:supervisor-review'
                UNION ALL SELECT 'president', 'salary:lock'
                UNION ALL SELECT 'vice-president', 'salary:review'
                UNION ALL SELECT 'vice-president', 'salary:supervisor-review'
                UNION ALL SELECT 'vice-president', 'salary:lock'
                UNION ALL SELECT 'finance-director', 'salary:review'
                UNION ALL SELECT 'finance-director', 'salary:submit-review'
                UNION ALL SELECT 'finance-manager', 'salary:review'
                UNION ALL SELECT 'finance-manager', 'salary:submit-review'
            ) v ON v.system_key = p.system_key
            WHERE NOT EXISTS (
                SELECT 1 FROM position_permissions existing
                WHERE existing.position_id = p.id AND existing.permission_key = v.permission_key
            ))SQL").execute();
        seedEmploymentPermissionKeysIfAbsent(dbManager, session);
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
                UNION ALL SELECT 'president', 'employment-assignment:approve'
                UNION ALL SELECT 'president', 'compensation:approve'
                UNION ALL SELECT 'president', 'attendance:read'
                UNION ALL SELECT 'president', 'attendance:manage'
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
                UNION ALL SELECT 'vice-president', 'employment-assignment:approve'
                UNION ALL SELECT 'vice-president', 'compensation:approve'
                UNION ALL SELECT 'vice-president', 'attendance:read'
                UNION ALL SELECT 'vice-president', 'attendance:manage'
                UNION ALL SELECT 'vice-president', 'scope:all'
                UNION ALL SELECT 'finance-director', 'portal:finance'
                UNION ALL SELECT 'finance-director', 'salary:read'
                UNION ALL SELECT 'finance-director', 'salary:write'
                UNION ALL SELECT 'finance-director', 'salary-profile:activate'
                UNION ALL SELECT 'finance-director', 'scope:all'
                UNION ALL SELECT 'finance-manager', 'portal:finance'
                UNION ALL SELECT 'finance-manager', 'salary:read'
                UNION ALL SELECT 'finance-manager', 'salary:write'
                UNION ALL SELECT 'finance-manager', 'salary-profile:activate'
                UNION ALL SELECT 'finance-manager', 'scope:all'
                UNION ALL SELECT 'department-manager', 'portal:super-admin'
                UNION ALL SELECT 'department-manager', 'portal:boss'
                UNION ALL SELECT 'department-manager', 'logs:read'
                UNION ALL SELECT 'department-manager', 'medical-record:read'
                UNION ALL SELECT 'department-manager', 'doctor-work:write'
                UNION ALL SELECT 'department-manager', 'user:delete'
                UNION ALL SELECT 'department-manager', 'employment-assignment:approve'
                UNION ALL SELECT 'department-manager', 'compensation:approve'
                UNION ALL SELECT 'department-manager', 'scope:all'
                UNION ALL SELECT 'super-admin', 'portal:super-admin'
                UNION ALL SELECT 'super-admin', 'logs:read'
                UNION ALL SELECT 'super-admin', 'medical-record:read'
                UNION ALL SELECT 'super-admin', 'doctor-work:write'
                UNION ALL SELECT 'super-admin', 'user:delete'
                UNION ALL SELECT 'super-admin', 'attendance:read'
                UNION ALL SELECT 'super-admin', 'scope:all'
                -- 超管巡检各业务端：授予除 boss 外的各端 portal 权限（不含 portal:boss，
                -- 否则登录落地端优先级会被抢到 /boss；这些权限优先级均低于 portal:super-admin，
                -- 超管默认仍落地 /super-admin，仅用于「快捷入口」跳入他端查功能）。
                UNION ALL SELECT 'super-admin', 'portal:medical'
                UNION ALL SELECT 'super-admin', 'portal:personnel'
                UNION ALL SELECT 'super-admin', 'portal:warehouse'
                UNION ALL SELECT 'super-admin', 'portal:finance'
                UNION ALL SELECT 'super-admin', 'portal:user'
                UNION ALL SELECT 'personnel-manager', 'portal:personnel'
                UNION ALL SELECT 'personnel-manager', 'staff-role:write'
                UNION ALL SELECT 'personnel-manager', 'employment:read'
                UNION ALL SELECT 'personnel-manager', 'employment:onboard'
                UNION ALL SELECT 'personnel-manager', 'employment:assign'
                UNION ALL SELECT 'personnel-manager', 'employment:regularize'
                UNION ALL SELECT 'personnel-manager', 'employment:offboard'
                UNION ALL SELECT 'personnel-manager', 'compensation:propose'
                UNION ALL SELECT 'personnel-manager', 'compensation:reassign-case'
                UNION ALL SELECT 'personnel-manager', 'attendance:read'
                UNION ALL SELECT 'personnel-manager', 'attendance:manage'
                UNION ALL SELECT 'personnel-manager', 'scope:all'
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

    void seedAllPositionPermissions(DatabaseManagerInterface &dbManager, mysqlx::Session &session)
    {
        seedPositionPermissions(dbManager, session);
        seedMedicalDocumentPermissionsIfAbsent(dbManager, session);
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
                UNION ALL SELECT 'Boss', 'employment-assignment:approve'
                UNION ALL SELECT 'Boss', 'compensation:approve'
                UNION ALL SELECT 'Boss', 'attendance:read'
                UNION ALL SELECT 'Boss', 'attendance:manage'
                UNION ALL SELECT 'Boss', 'scope:all'
                UNION ALL SELECT 'Finance', 'portal:finance'
                UNION ALL SELECT 'Finance', 'salary:read'
                UNION ALL SELECT 'Finance', 'salary:write'
                UNION ALL SELECT 'Finance', 'salary-profile:activate'
                UNION ALL SELECT 'Finance', 'scope:all'
                UNION ALL SELECT 'SuperAdmin', 'portal:super-admin'
                UNION ALL SELECT 'SuperAdmin', 'logs:read'
                UNION ALL SELECT 'SuperAdmin', 'medical-record:read'
                UNION ALL SELECT 'SuperAdmin', 'doctor-work:write'
                UNION ALL SELECT 'SuperAdmin', 'user:delete'
                UNION ALL SELECT 'SuperAdmin', 'attendance:read'
                UNION ALL SELECT 'SuperAdmin', 'scope:all'
                UNION ALL SELECT 'Personnel', 'portal:personnel'
                UNION ALL SELECT 'Personnel', 'staff-role:write'
                UNION ALL SELECT 'Personnel', 'employment:read'
                UNION ALL SELECT 'Personnel', 'employment:onboard'
                UNION ALL SELECT 'Personnel', 'employment:assign'
                UNION ALL SELECT 'Personnel', 'employment:regularize'
                UNION ALL SELECT 'Personnel', 'employment:offboard'
                UNION ALL SELECT 'Personnel', 'compensation:propose'
                UNION ALL SELECT 'Personnel', 'compensation:reassign-case'
                UNION ALL SELECT 'Personnel', 'attendance:read'
                UNION ALL SELECT 'Personnel', 'attendance:manage'
                UNION ALL SELECT 'Personnel', 'scope:all'
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

    void seedMedicalDocumentTemplatePermissionsIfAbsent(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql(R"SQL(INSERT IGNORE INTO permission_template_items (template_id, permission_key)
            SELECT t.id, v.permission_key
            FROM permission_templates t
            JOIN (
                SELECT 'Boss' AS template_name, 'medical-record:finalize' AS permission_key
                UNION ALL SELECT 'Boss', 'medical-record:print'
                UNION ALL SELECT 'Boss', 'medical-record:amend'
                UNION ALL SELECT 'Boss', 'medical-record:void'
                UNION ALL SELECT 'Boss', 'report-template:read'
                UNION ALL SELECT 'Boss', 'report-template:manage'
                UNION ALL SELECT 'Boss', 'report-template:publish'
                UNION ALL SELECT 'SuperAdmin', 'medical-record:print'
                UNION ALL SELECT 'SuperAdmin', 'report-template:read'
                UNION ALL SELECT 'SuperAdmin', 'report-template:manage'
                UNION ALL SELECT 'SuperAdmin', 'report-template:publish'
                UNION ALL SELECT 'Medical', 'medical-record:finalize'
                UNION ALL SELECT 'Medical', 'medical-record:print'
                UNION ALL SELECT 'Medical', 'medical-record:amend'
                UNION ALL SELECT 'Medical', 'medical-record:void'
            ) v ON v.template_name = t.name
            WHERE NOT EXISTS (
                SELECT 1 FROM permission_template_items existing
                WHERE existing.template_id = t.id AND existing.permission_key = v.permission_key
            ))SQL")
            .execute();
    }

    void seedAllPermissionTemplateItems(DatabaseManagerInterface &dbManager, mysqlx::Session &session)
    {
        seedPermissionTemplateItems(dbManager, session);
        seedMedicalDocumentTemplatePermissionsIfAbsent(dbManager, session);
        session.sql(R"SQL(INSERT IGNORE INTO permission_template_items (template_id, permission_key)
            SELECT t.id, v.permission_key
            FROM permission_templates t
            JOIN (
                SELECT 'Boss' AS template_name, 'salary:review' AS permission_key
                UNION ALL SELECT 'Boss', 'salary:supervisor-review'
                UNION ALL SELECT 'Boss', 'salary:lock'
                UNION ALL SELECT 'Finance', 'salary:review'
                UNION ALL SELECT 'Finance', 'salary:submit-review'
            ) v ON v.template_name = t.name
            WHERE NOT EXISTS (
                SELECT 1 FROM permission_template_items existing
                WHERE existing.template_id = t.id AND existing.permission_key = v.permission_key
            ))SQL").execute();
    }

    void migratePayrollPeriodColumns(DatabaseManagerInterface &dbManager, mysqlx::Session &)
    {
        Columns::migratePayrollPeriod(dbManager);
    }

    const char *defaultMedicalDocumentTemplate()
    {
        return R"HTML(<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8"><style>
@page{size:A4;margin:14mm 15mm 16mm}*{box-sizing:border-box}body{margin:0;color:#172b2d;font-family:"PingFang SC","Microsoft YaHei",sans-serif;font-size:12px;line-height:1.65}.header{display:flex;justify-content:space-between;align-items:flex-end;border-bottom:2px solid #1d6b62;padding-bottom:12px;margin-bottom:16px}.brand{font-size:22px;font-weight:700}.title{font-size:18px;font-weight:700}.meta{color:#607472}.grid{display:grid;grid-template-columns:repeat(4,1fr);border:1px solid #b8c8c5;border-bottom:0}.cell{padding:7px 9px;border-right:1px solid #b8c8c5;border-bottom:1px solid #b8c8c5}.cell:nth-child(4n){border-right:0}.label{display:block;color:#6b7d7a;font-size:10px}.section{margin-top:14px}.section h2{margin:0 0 6px;font-size:13px;color:#1d6b62}.content{min-height:34px;padding:8px 10px;border:1px solid #cbd7d5;white-space:pre-wrap}.rx{width:100%;border-collapse:collapse}.rx th,.rx td{padding:7px;border:1px solid #b8c8c5;text-align:left}.rx th{background:#edf5f2;color:#315f59}.footer{display:flex;justify-content:space-between;margin-top:22px;padding-top:10px;border-top:1px solid #b8c8c5}.muted{color:#6b7d7a}
</style></head><body>
<header class="header"><div><div class="brand">{{hospital.name}}</div><div class="meta">{{hospital.address}} {{hospital.phone}}</div></div><div><div class="title">诊疗记录单</div><div class="meta">编号 {{document.number}}</div></div></header>
<section class="grid"><div class="cell"><span class="label">宠物姓名</span>{{pet.name}}</div><div class="cell"><span class="label">种类 / 品种</span>{{pet.species}} / {{pet.breed}}</div><div class="cell"><span class="label">性别 / 年龄</span>{{pet.sex}} / {{pet.age}}</div><div class="cell"><span class="label">主人</span>{{owner.name}}</div><div class="cell"><span class="label">接诊医生</span>{{doctor.name}}</div><div class="cell"><span class="label">接诊时间</span>{{document.issuedAt}}</div><div class="cell"><span class="label">订单编号</span>{{document.orderNumber}}</div><div class="cell"><span class="label">状态</span>{{document.status}}</div></section>
<section class="section"><h2>主诉</h2><div class="content">{{visit.chiefComplaint}}</div></section><section class="section"><h2>现病史与既往史</h2><div class="content">{{visit.presentIllness}}\n{{visit.pastHistory}}</div></section><section class="section"><h2>检查记录</h2><div class="content">{{visit.physicalExam}}</div></section><section class="section"><h2>诊断</h2><div class="content">{{visit.diagnosis}}</div></section><section class="section"><h2>治疗方案</h2><div class="content">{{visit.treatmentPlan}}</div></section>
<section class="section"><h2>处方与用药</h2><table class="rx"><thead><tr><th>药品</th><th>剂量</th><th>频次</th><th>途径</th><th>疗程</th><th>数量</th><th>说明</th></tr></thead><tbody>{{#prescription.items}}<tr><td>{{medicineName}}</td><td>{{dosage}}</td><td>{{frequency}}</td><td>{{route}}</td><td>{{durationDays}} 天</td><td>{{quantity}} {{unit}}</td><td>{{instructions}}</td></tr>{{/prescription.items}}</tbody></table></section>
<section class="section"><h2>离院医嘱与复诊</h2><div class="content">{{visit.dischargeAdvice}}\n复诊时间：{{visit.followUpAt}}</div></section><footer class="footer"><span class="muted">本诊疗单由 PetManager 生成</span><span>医师签名：{{doctor.name}}</span></footer>
</body></html>)HTML";
    }

    void seedDefaultReportTemplate(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql("INSERT IGNORE INTO report_templates (code, name, document_type, paper_size, orientation, status, created_by) "
                    "VALUES ('medical-document-a4', '默认 A4 诊疗单', 'medical_document', 'A4', 'portrait', 'published', NULL)")
            .execute();
    }

    void seedDefaultReportTemplateVersion(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        session.sql("INSERT INTO report_template_versions "
                    "(template_id, version_no, engine, template_content, data_contract_version, status, published_at) "
                    "SELECT id, 1, 'html', ?, 'medical-document.v1', 'published', UTC_TIMESTAMP() "
                    "FROM report_templates t WHERE code = 'medical-document-a4' "
                    "AND NOT EXISTS (SELECT 1 FROM report_template_versions v WHERE v.template_id=t.id AND v.version_no=1) LIMIT 1")
            .bind(defaultMedicalDocumentTemplate())
            .execute();
        session.sql("UPDATE report_templates t JOIN report_template_versions v ON v.template_id = t.id "
                    "SET t.current_version_id = v.id WHERE t.code = 'medical-document-a4' "
                    "AND v.version_no = 1 AND t.current_version_id IS NULL")
            .execute();
    }

    void ensureReportTemplateCurrentVersionForeignKey(DatabaseManagerInterface &dbManager, mysqlx::Session &session)
    {
        session.sql("UPDATE report_templates t "
                    "LEFT JOIN report_template_versions v ON v.id=t.current_version_id AND v.template_id=t.id "
                    "SET t.current_version_id=NULL "
                    "WHERE t.current_version_id IS NOT NULL AND v.id IS NULL")
            .execute();
        seedDefaultReportTemplateVersion(dbManager, session);
        if (!Common::foreignKeyExists(dbManager, "report_templates", "fk_report_templates_current_version"))
        {
            session.sql("ALTER TABLE report_templates "
                        "ADD CONSTRAINT fk_report_templates_current_version "
                        "FOREIGN KEY (current_version_id) REFERENCES report_template_versions(id) ON DELETE RESTRICT")
                .execute();
        }
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

        // 业务域：部门天然属于某业务域，用于约束该部门下可创建的职位工种（杜绝「管理部·财务」）。
        // 取值与 Permissions::domainKey 一致：general/management/finance/personnel/medical/warehouse。
        const bool domainColumnAdded =
            Common::addColumnIfNotExists(dbManager, "departments", "business_domain", "VARCHAR(32) NOT NULL DEFAULT 'general'");
        // 回填：种子部门的 system_key 恰为域 key，直接采用；其余按已有职位的工种推断。
        session.sql("UPDATE departments SET business_domain = system_key "
                    "WHERE business_domain = 'general' "
                    "AND system_key IN ('management','finance','personnel','medical','warehouse')")
            .execute();
        session.sql(
            "UPDATE departments d SET business_domain = ("
            "  SELECT CASE"
            "    WHEN SUM(p.staff_kind IN ('doctor','nurse')) > 0 THEN 'medical'"
            "    WHEN SUM(p.staff_kind = 'finance') > 0 THEN 'finance'"
            "    WHEN SUM(p.staff_kind = 'personnel') > 0 THEN 'personnel'"
            "    WHEN SUM(p.staff_kind = 'warehouse') > 0 THEN 'warehouse'"
            "    WHEN SUM(p.staff_kind = 'management') > 0 THEN 'management'"
            "    ELSE 'general' END"
            "  FROM positions p WHERE p.department_id = d.id)"
            " WHERE d.business_domain = 'general' "
            " AND EXISTS (SELECT 1 FROM positions p2 WHERE p2.department_id = d.id "
            "             AND p2.staff_kind IN ('doctor','nurse','finance','personnel','warehouse','management'))")
            .execute();
        (void)domainColumnAdded;
    }

    void migratePositionDescriptionColumn(DatabaseManagerInterface &dbManager, mysqlx::Session &session)
    {
        Common::addColumnIfNotExists(dbManager, "positions", "description", "VARCHAR(255) NOT NULL DEFAULT ''");
        // 派岗策略列：默认 super_admin_only（fail-closed）。
        Common::addColumnIfNotExists(
            dbManager,
            "positions",
            "assignment_policy",
            "ENUM('personnel_direct','approval_required','super_admin_only') NOT NULL DEFAULT 'super_admin_only'");

        // 系统 seed 初始策略（非安全判断清单）：锚点职位合理默认值。
        // 随后由权威 Permissions catalog 仅抬升，不自动降级。
        session.sql("UPDATE positions SET assignment_policy = 'super_admin_only' "
                    "WHERE system_key = 'super-admin'")
            .execute();
        session.sql("UPDATE positions SET assignment_policy = 'approval_required' "
                    "WHERE system_key IN ("
                    "'president','vice-president','department-manager',"
                    "'finance-director','finance-manager','personnel-manager'"
                    ") AND assignment_policy = 'personnel_direct'")
            .execute();
        session.sql("UPDATE positions SET assignment_policy = 'personnel_direct' "
                    "WHERE system_key IN ('doctor','nurse','warehouse-admin') "
                    "AND assignment_policy = 'super_admin_only'")
            .execute();

        // B17: 安全下限唯一来源 = 权威 catalog 的 requiredAssignmentPolicy(原始 DB keys)。
        // 未知 key fail-closed → SuperAdminOnly；只抬升 stored policy，不降级。
        // super-admin 永远 super_admin_only。
        // B18: 先完整消费 positions 结果集到本地 vector，再逐项查询/更新，避免同一 Session
        // 在活动 SqlResult 未耗尽时嵌套执行（MySQL X 不可重入）。
        try
        {
            struct PositionPolicyRow
            {
                int id = 0;
                std::string systemKey;
                std::string storedPolicy;
            };
            std::vector<PositionPolicyRow> positionRows;
            {
                mysqlx::SqlResult positions = session.sql(
                                                         "SELECT id, COALESCE(system_key, ''), "
                                                         "COALESCE(assignment_policy, 'super_admin_only') "
                                                         "FROM positions")
                                                  .execute();
                for (mysqlx::Row pos = positions.fetchOne(); pos; pos = positions.fetchOne())
                {
                    if (pos[0].isNull())
                    {
                        continue;
                    }
                    PositionPolicyRow row;
                    row.id = pos[0].get<int>();
                    row.systemKey = pos[1].isNull() ? "" : pos[1].get<std::string>();
                    row.storedPolicy = pos[2].isNull() ? "super_admin_only" : pos[2].get<std::string>();
                    positionRows.push_back(std::move(row));
                }
            } // positions SqlResult 已离开作用域 / 结果集已完整消费

            for (const PositionPolicyRow &pos : positionRows)
            {
                if (pos.systemKey == "super-admin")
                {
                    session.sql("UPDATE positions SET assignment_policy = 'super_admin_only' WHERE id = ?")
                        .bind(pos.id)
                        .execute();
                    continue;
                }

                std::vector<std::string> rawKeys;
                mysqlx::SqlResult perms = session.sql(
                                                    "SELECT permission_key FROM position_permissions WHERE position_id = ?")
                                             .bind(pos.id)
                                             .execute();
                for (mysqlx::Row keyRow = perms.fetchOne(); keyRow; keyRow = perms.fetchOne())
                {
                    if (!keyRow[0].isNull())
                    {
                        rawKeys.push_back(keyRow[0].get<std::string>());
                    }
                }

                const auto floor = Permissions::requiredAssignmentPolicy(rawKeys);
                const auto stored = Permissions::parseAssignmentPolicy(pos.storedPolicy);
                const auto raised = Permissions::maxAssignmentPolicy(stored, floor);
                if (Permissions::assignmentPolicyRank(raised) > Permissions::assignmentPolicyRank(stored))
                {
                    session.sql("UPDATE positions SET assignment_policy = ? WHERE id = ?")
                        .bind(Permissions::assignmentPolicyKey(raised), pos.id)
                        .execute();
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "assignment_policy catalog raise failed: " << e.what() << std::endl;
            throw;
        }
    }

    std::string generateBootstrapPassword()
    {
        std::array<unsigned char, 16> bytes{};
        if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1)
        {
            throw std::runtime_error("Failed to generate bootstrap administrator password");
        }

        std::ostringstream password;
        password << "Pm!";
        for (const unsigned char byte : bytes)
        {
            password << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return password.str();
    }

    void ensureBootstrapSuperAdmin(DatabaseManagerInterface &, mysqlx::Session &session)
    {
        mysqlx::Row userCountRow = session.sql("SELECT COUNT(*) FROM users").execute().fetchOne();
        if (userCountRow && !userCountRow[0].isNull() && userCountRow[0].get<int>() > 0)
        {
            return;
        }

        const char *passwordEnv = std::getenv("PETMANAGER_BOOTSTRAP_ADMIN_PASSWORD");
        const bool generatedPassword = passwordEnv == nullptr || std::string(passwordEnv).empty();
        const std::string password = generatedPassword ? generateBootstrapPassword() : std::string(passwordEnv);

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
                    "VALUES ('staff', ?, '系统管理员', ?, ?)")
            .bind(positionRow[0].get<int>(), hash_password(password), email)
            .execute();
        std::cout << "\n============================================================\n"
                  << "Bootstrap super-admin created\n"
                  << "Email: " << email << "\n";
        if (generatedPassword)
        {
            std::cout << "Temporary password: " << password << "\n"
                      << "Store this password now; it will not be shown again.\n";
        }
        else
        {
            std::cout << "Password: configured by PETMANAGER_BOOTSTRAP_ADMIN_PASSWORD\n";
        }
        std::cout << "============================================================\n"
                  << std::endl;
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
                assignment_policy ENUM('personnel_direct','approval_required','super_admin_only') NOT NULL DEFAULT 'super_admin_only',
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
            seedAllPositionPermissions,
            seedIncrementalPositionPermissions,
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
            seedAllPermissionTemplateItems,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &session) {
                seedMedicalDocumentTemplatePermissionsIfAbsent(dbManager, session);
                session.sql(R"SQL(INSERT IGNORE INTO permission_template_items (template_id, permission_key)
                    SELECT t.id, v.permission_key FROM permission_templates t
                    JOIN (
                        SELECT 'Boss' AS template_name, 'salary:review' AS permission_key
                        UNION ALL SELECT 'Boss', 'salary:supervisor-review'
                        UNION ALL SELECT 'Boss', 'salary:lock'
                        UNION ALL SELECT 'Finance', 'salary:review'
                        UNION ALL SELECT 'Finance', 'salary:submit-review'
                    ) v ON v.template_name=t.name
                    WHERE NOT EXISTS (SELECT 1 FROM permission_template_items e
                        WHERE e.template_id=t.id AND e.permission_key=v.permission_key))SQL").execute();
            },
        },
        {
            "financialRecord",
            R"SQL(CREATE TABLE financialRecord (
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
                INDEX idx_financialRecord_is_deleted (is_deleted)
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
                attendance_no VARCHAR(32) NULL,
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
                UNIQUE KEY uq_users_attendance_no (attendance_no),
                INDEX idx_users_position_id (position_id),
                INDEX idx_users_account_type (account_type),
                INDEX idx_users_is_deleted (is_deleted)
            ))SQL",
            ensureBootstrapSuperAdmin,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                // 先建 bootstrap 管理员再跑列迁移：migrateUsers 里的 attendance_no 回填
                // 才能覆盖到它，否则要等下一次启动。
                ensureBootstrapSuperAdmin(dbManager, *dbManager.getSession());
                Columns::migrateUsers(dbManager);
                ForeignKeys::migrateUsers(dbManager);
            },
        },
        {
            "employment",
            R"SQL(CREATE TABLE employment (
                id BIGINT PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL UNIQUE,
                status ENUM(
                    'draft', 'onboarding', 'probation',
                    'regularization_pending', 'active',
                    'rejected', 'separated'
                ) NOT NULL DEFAULT 'draft',
                hire_date DATE NULL,
                probation_start DATE NULL,
                probation_end DATE NULL,
                probation_waived TINYINT NOT NULL DEFAULT 0,
                legacy_imported TINYINT NOT NULL DEFAULT 0,
                regularized_at DATETIME NULL,
                separated_at DATETIME NULL,
                row_version INT NOT NULL DEFAULT 1,
                created_by INT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_employment_user FOREIGN KEY (user_id) REFERENCES users(id),
                CONSTRAINT fk_employment_created_by FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            [](DatabaseManagerInterface &, mysqlx::Session &session) {
                // 现有 staff 建 active employment + legacy 任职历史（request_source=migration）
                session.sql(R"SQL(
                    INSERT INTO employment (user_id, status, hire_date, legacy_imported, created_by)
                    SELECT u.id, 'active', DATE(u.created_at), 1, NULL
                    FROM users u
                    WHERE u.account_type = 'staff' AND u.is_deleted = 0
                      AND NOT EXISTS (SELECT 1 FROM employment e WHERE e.user_id = u.id)
                )SQL").execute();
            },
            [](DatabaseManagerInterface &, mysqlx::Session &session) {
                session.sql(R"SQL(
                    INSERT INTO employment (user_id, status, hire_date, legacy_imported, created_by)
                    SELECT u.id, 'active', DATE(u.created_at), 1, NULL
                    FROM users u
                    WHERE u.account_type = 'staff' AND u.is_deleted = 0
                      AND NOT EXISTS (SELECT 1 FROM employment e WHERE e.user_id = u.id)
                )SQL").execute();
            },
        },
        {
            "employment_assignment",
            R"SQL(CREATE TABLE employment_assignment (
                id BIGINT PRIMARY KEY AUTO_INCREMENT,
                employment_id BIGINT NOT NULL,
                branch_id INT NOT NULL,
                department_id INT NOT NULL,
                from_position_id INT NULL,
                to_position_id INT NULL,
                action ENUM('onboard','transfer','regularize','offboard') NOT NULL,
                status ENUM('pending','approved','rejected','effective','cancelled') NOT NULL,
                effective_from DATE NOT NULL,
                reason VARCHAR(500) NOT NULL,
                request_source ENUM('user','migration') NOT NULL DEFAULT 'user',
                requested_by INT NULL,
                migration_batch_id VARCHAR(64) NULL,
                reviewed_by INT NULL,
                reviewed_at DATETIME NULL,
                expected_employment_row_version INT NOT NULL,
                row_version INT NOT NULL DEFAULT 1,
                open_slot TINYINT GENERATED ALWAYS AS (
                    CASE WHEN status IN ('pending','approved') THEN 1 ELSE NULL END
                ) STORED,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                UNIQUE KEY uq_employment_assignment_open (employment_id, open_slot),
                INDEX idx_employment_assignment_scope_status (branch_id, department_id, status),
                CONSTRAINT fk_ea_employment FOREIGN KEY (employment_id) REFERENCES employment(id) ON DELETE CASCADE,
                CONSTRAINT fk_ea_branch FOREIGN KEY (branch_id) REFERENCES branches(id),
                CONSTRAINT fk_ea_department FOREIGN KEY (department_id) REFERENCES departments(id),
                CONSTRAINT fk_ea_from_position FOREIGN KEY (from_position_id) REFERENCES positions(id),
                CONSTRAINT fk_ea_to_position FOREIGN KEY (to_position_id) REFERENCES positions(id),
                CONSTRAINT fk_ea_requested_by FOREIGN KEY (requested_by) REFERENCES users(id),
                CONSTRAINT fk_ea_reviewed_by FOREIGN KEY (reviewed_by) REFERENCES users(id),
                CONSTRAINT chk_ea_request_source CHECK (
                    (request_source='user' AND requested_by IS NOT NULL AND migration_batch_id IS NULL) OR
                    (request_source='migration' AND requested_by IS NULL AND migration_batch_id IS NOT NULL)
                ),
                CONSTRAINT chk_ea_separation_of_duties CHECK (
                    reviewed_by IS NULL OR requested_by IS NULL OR reviewed_by <> requested_by
                )
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            [](DatabaseManagerInterface &, mysqlx::Session &session) {
                session.sql(R"SQL(
                    INSERT INTO employment_assignment (
                        employment_id, branch_id, department_id, from_position_id, to_position_id,
                        action, status, effective_from, reason, request_source, requested_by,
                        migration_batch_id, reviewed_by, reviewed_at, expected_employment_row_version)
                    SELECT e.id, d.branch_id, p.department_id, NULL, u.position_id,
                           'onboard', 'effective', DATE(u.created_at),
                           'Legacy employment import', 'migration', NULL,
                           'legacy-employment-v1', NULL, NULL, e.row_version
                    FROM employment e
                    JOIN users u ON u.id = e.user_id
                    JOIN positions p ON p.id = u.position_id
                    JOIN departments d ON d.id = p.department_id
                    WHERE e.legacy_imported = 1
                      AND NOT EXISTS (
                        SELECT 1 FROM employment_assignment ea WHERE ea.employment_id = e.id
                      )
                )SQL").execute();
            },
            [](DatabaseManagerInterface &, mysqlx::Session &session) {
                session.sql(R"SQL(
                    INSERT INTO employment_assignment (
                        employment_id, branch_id, department_id, from_position_id, to_position_id,
                        action, status, effective_from, reason, request_source, requested_by,
                        migration_batch_id, reviewed_by, reviewed_at, expected_employment_row_version)
                    SELECT e.id, d.branch_id, p.department_id, NULL, u.position_id,
                           'onboard', 'effective', DATE(u.created_at),
                           'Legacy employment import', 'migration', NULL,
                           'legacy-employment-v1', NULL, NULL, e.row_version
                    FROM employment e
                    JOIN users u ON u.id = e.user_id
                    JOIN positions p ON p.id = u.position_id
                    JOIN departments d ON d.id = p.department_id
                    WHERE e.legacy_imported = 1
                      AND u.account_type = 'staff'
                      AND u.position_id IS NOT NULL
                      AND NOT EXISTS (
                        SELECT 1 FROM employment_assignment ea WHERE ea.employment_id = e.id
                      )
                )SQL").execute();
            },
        },
        {
            "employment_event_outbox",
            R"SQL(CREATE TABLE employment_event_outbox (
                id BIGINT PRIMARY KEY AUTO_INCREMENT,
                event_key VARCHAR(96) NOT NULL UNIQUE,
                employment_id BIGINT NOT NULL,
                user_id INT NOT NULL,
                event_type ENUM('assignment_changed','employment_separated') NOT NULL,
                payload JSON NOT NULL,
                status ENUM('pending','processing','completed','failed') NOT NULL DEFAULT 'pending',
                attempts INT NOT NULL DEFAULT 0,
                last_error VARCHAR(1000) NOT NULL DEFAULT '',
                next_attempt_at DATETIME NULL,
                locked_at DATETIME NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                completed_at DATETIME NULL,
                INDEX idx_employment_outbox_dispatch (status, next_attempt_at),
                INDEX idx_employment_outbox_locked (status, locked_at),
                CONSTRAINT fk_employment_outbox_employment FOREIGN KEY (employment_id) REFERENCES employment(id),
                CONSTRAINT fk_employment_outbox_user FOREIGN KEY (user_id) REFERENCES users(id)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &) {
                Common::addColumnIfNotExists(
                    dbManager, "employment_event_outbox", "locked_at", "DATETIME NULL");
                Common::addIndexIfNotExists(
                    dbManager, "employment_event_outbox", "idx_employment_outbox_locked",
                    "locked_at");
            },
        },
        {
            "employment_workflow_audit",
            R"SQL(CREATE TABLE employment_workflow_audit (
                id BIGINT PRIMARY KEY AUTO_INCREMENT,
                resource_type ENUM('employment','assignment','compensation') NOT NULL,
                resource_id BIGINT NOT NULL,
                action VARCHAR(64) NOT NULL,
                operator_id INT NULL,
                branch_id INT NULL,
                department_id INT NULL,
                before_snapshot JSON NULL,
                after_snapshot JSON NULL,
                reason VARCHAR(1000) NOT NULL DEFAULT '',
                request_id VARCHAR(64) NOT NULL DEFAULT '',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                INDEX idx_workflow_audit_resource (resource_type, resource_id, created_at),
                INDEX idx_workflow_audit_operator (operator_id, created_at),
                CONSTRAINT fk_workflow_audit_operator FOREIGN KEY (operator_id) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
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
            "user_permissions",
            R"SQL(CREATE TABLE user_permissions (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL,
                permission_key VARCHAR(64) NOT NULL,
                granted_by INT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                UNIQUE KEY uq_user_permission (user_id, permission_key),
                INDEX idx_user_permissions_user_id (user_id),
                CONSTRAINT fk_user_permission_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                CONSTRAINT fk_user_permission_granted_by FOREIGN KEY (granted_by) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
        },
        {
            "attendance_devices",
            R"SQL(CREATE TABLE attendance_devices (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                name VARCHAR(100) NOT NULL,
                device_key VARCHAR(64) NOT NULL,
                hmac_key_cipher VARCHAR(255) NOT NULL,
                vendor VARCHAR(64) NOT NULL DEFAULT '',
                location VARCHAR(128) NOT NULL DEFAULT '',
                branch_id INT NULL,
                is_active TINYINT NOT NULL DEFAULT 1,
                last_seen_at DATETIME NULL,
                created_by INT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                UNIQUE KEY uq_attendance_devices_device_key (device_key),
                INDEX idx_attendance_devices_branch (branch_id),
                CONSTRAINT fk_attendance_device_branch FOREIGN KEY (branch_id) REFERENCES branches(id),
                CONSTRAINT fk_attendance_device_created_by FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
        },
        {
            "attendance_workdays",
            R"SQL(CREATE TABLE attendance_workdays (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                work_date DATE NOT NULL,
                day_type ENUM('workday','holiday') NOT NULL DEFAULT 'workday',
                check_in_start TIME NOT NULL DEFAULT '08:00:00',
                check_in_end TIME NOT NULL DEFAULT '09:00:00',
                check_out_start TIME NOT NULL DEFAULT '18:00:00',
                check_out_end TIME NOT NULL DEFAULT '23:59:59',
                note VARCHAR(255) NOT NULL DEFAULT '',
                created_by INT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                UNIQUE KEY uq_attendance_workdays_date (work_date),
                CONSTRAINT fk_attendance_workday_created_by FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
        },
        {
            "attendance_punches",
            R"SQL(CREATE TABLE attendance_punches (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                device_id INT NULL,
                user_id INT NOT NULL,
                attendance_no VARCHAR(32) NOT NULL DEFAULT '',
                event_id VARCHAR(128) NULL,
                raw_event_hash CHAR(64) NOT NULL,
                punched_at DATETIME NOT NULL,
                verify_mode ENUM('face','fingerprint','card','password','manual','unknown') NOT NULL DEFAULT 'unknown',
                source ENUM('device','manual','import') NOT NULL DEFAULT 'device',
                is_voided TINYINT NOT NULL DEFAULT 0,
                voided_by INT NULL,
                voided_at DATETIME NULL,
                void_reason VARCHAR(255) NOT NULL DEFAULT '',
                created_by INT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                UNIQUE KEY uq_attendance_punch_event (device_id, event_id),
                UNIQUE KEY uq_attendance_punch_hash (device_id, raw_event_hash),
                INDEX idx_attendance_punch_user_time (user_id, punched_at),
                INDEX idx_attendance_punch_attendance_no_time (attendance_no, punched_at),
                INDEX idx_attendance_punch_voided (is_voided),
                CONSTRAINT fk_attendance_punch_device FOREIGN KEY (device_id) REFERENCES attendance_devices(id) ON DELETE SET NULL,
                CONSTRAINT fk_attendance_punch_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                CONSTRAINT fk_attendance_punch_voided_by FOREIGN KEY (voided_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT fk_attendance_punch_created_by FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
        },
        {
            "attendance_records",
            R"SQL(CREATE TABLE attendance_records (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL,
                work_date DATE NOT NULL,
                check_in_at DATETIME NULL,
                check_out_at DATETIME NULL,
                status ENUM('normal','late','early_leave','late_and_early','missing_out','absent') NOT NULL DEFAULT 'absent',
                is_corrected TINYINT NOT NULL DEFAULT 0,
                corrected_by INT NULL,
                corrected_at DATETIME NULL,
                correction_note VARCHAR(255) NOT NULL DEFAULT '',
                branch_id INT NULL,
                department_id INT NULL,
                position_id INT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                UNIQUE KEY uq_attendance_records_user_date (user_id, work_date),
                INDEX idx_attendance_records_date (work_date),
                INDEX idx_attendance_records_department_date (department_id, work_date),
                INDEX idx_attendance_records_branch_date (branch_id, work_date),
                CONSTRAINT fk_attendance_record_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                CONSTRAINT fk_attendance_record_corrected_by FOREIGN KEY (corrected_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT fk_attendance_record_branch FOREIGN KEY (branch_id) REFERENCES branches(id),
                CONSTRAINT fk_attendance_record_department FOREIGN KEY (department_id) REFERENCES departments(id),
                CONSTRAINT fk_attendance_record_position FOREIGN KEY (position_id) REFERENCES positions(id)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
        },
        {
            "device_person_sync",
            R"SQL(CREATE TABLE device_person_sync (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                device_id INT NOT NULL,
                user_id INT NOT NULL,
                attendance_no VARCHAR(32) NOT NULL,
                desired_state ENUM('active','deleted') NOT NULL DEFAULT 'active',
                sync_state ENUM('pending','synced','failed') NOT NULL DEFAULT 'pending',
                last_error VARCHAR(255) NOT NULL DEFAULT '',
                last_synced_at DATETIME NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                UNIQUE KEY uq_device_person_sync (device_id, user_id),
                INDEX idx_device_person_sync_state (device_id, sync_state),
                CONSTRAINT fk_device_person_sync_device FOREIGN KEY (device_id) REFERENCES attendance_devices(id) ON DELETE CASCADE,
                CONSTRAINT fk_device_person_sync_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
        },
        {
            "attendance_device_events",
            R"SQL(CREATE TABLE attendance_device_events (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                device_id INT NULL,
                device_key VARCHAR(64) NOT NULL DEFAULT '',
                event_type VARCHAR(64) NOT NULL,
                result ENUM('success','failure') NOT NULL DEFAULT 'success',
                request_hash CHAR(64) NOT NULL DEFAULT '',
                message VARCHAR(255) NOT NULL DEFAULT '',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                INDEX idx_attendance_device_events_device_time (device_id, created_at),
                INDEX idx_attendance_device_events_type_time (event_type, created_at),
                CONSTRAINT fk_attendance_device_event_device FOREIGN KEY (device_id) REFERENCES attendance_devices(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                // 未注册设备 device_id 为 NULL，排查"哪台设备在乱发"全靠 device_key；已建表的库补列。
                Common::addColumnIfNotExists(dbManager, "attendance_device_events", "device_key",
                                             "VARCHAR(64) NOT NULL DEFAULT '' AFTER device_id");
            },
        },
        {
            "attendance_device_nonces",
            R"SQL(CREATE TABLE attendance_device_nonces (
                device_id INT NOT NULL,
                nonce VARCHAR(128) NOT NULL,
                expires_at DATETIME NOT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                PRIMARY KEY (device_id, nonce),
                INDEX idx_attendance_device_nonces_expires (expires_at),
                CONSTRAINT fk_attendance_device_nonce_device FOREIGN KEY (device_id) REFERENCES attendance_devices(id) ON DELETE CASCADE
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
            "salaryProfile",
            R"SQL(CREATE TABLE salaryProfile (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                user_id INT NOT NULL,
                pay_type ENUM('monthly', 'hourly') NOT NULL DEFAULT 'monthly' COMMENT '计薪方式：月薪或时薪',
                base_salary DECIMAL(18, 2) NULL COMMENT '月薪工基础薪资',
                hourly_rate DECIMAL(18, 2) NULL COMMENT '时薪工时效薪资',
                social_insurance_housing_fund DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '默认五险一金扣款金额',
                effective_from DATE NOT NULL COMMENT '配置生效日期',
                effective_to DATE NULL COMMENT '配置失效日期，NULL 表示当前有效',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_salaryProfile_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                CONSTRAINT chk_salaryProfile_pay_basis CHECK (
                    (pay_type = 'monthly' AND base_salary IS NOT NULL AND base_salary >= 0 AND hourly_rate IS NULL) OR
                    (pay_type = 'hourly' AND hourly_rate IS NOT NULL AND hourly_rate >= 0 AND base_salary IS NULL)
                ),
                CONSTRAINT chk_salaryProfile_social_insurance CHECK (social_insurance_housing_fund >= 0),
                UNIQUE KEY uq_salaryProfile_user_effective_from (user_id, effective_from),
                INDEX idx_salaryProfile_user_effective (user_id, effective_from, effective_to),
                INDEX idx_salaryProfile_pay_type (pay_type)
            ))SQL",
            nullptr,
            nullptr,
        },
        {
            // v6 薪酬提案：人事拟案 → 管理批准 → 财务激活；不改写存量 salaryProfile 金额。
            "compensation_proposal",
            R"SQL(CREATE TABLE compensation_proposal (
                id BIGINT PRIMARY KEY AUTO_INCREMENT,
                employment_id BIGINT NOT NULL,
                branch_id INT NOT NULL COMMENT '创建时组织快照',
                department_id INT NOT NULL COMMENT '创建时组织快照',
                phase ENUM('probation','regular','adjustment') NOT NULL,
                pay_type ENUM('monthly','hourly') NOT NULL,
                base_salary DECIMAL(18, 2) NULL,
                hourly_rate DECIMAL(18, 2) NULL,
                social_insurance_housing_fund DECIMAL(18, 2) NOT NULL DEFAULT 0.00,
                effective_from DATE NOT NULL,
                status ENUM(
                    'draft','submitted','management_approved','returned',
                    'finance_confirmed','active','cancelled'
                ) NOT NULL DEFAULT 'draft',
                assignee_user_id INT NOT NULL,
                proposed_by INT NOT NULL,
                submitted_by INT NULL,
                submitted_at DATETIME NULL,
                approved_by INT NULL,
                approved_at DATETIME NULL,
                finance_confirmed_by INT NULL,
                finance_confirmed_at DATETIME NULL,
                salary_profile_id INT NULL,
                note VARCHAR(1000) NOT NULL DEFAULT '',
                expected_employment_row_version INT NULL,
                row_version INT NOT NULL DEFAULT 1,
                open_slot TINYINT GENERATED ALWAYS AS (
                    CASE WHEN status IN (
                        'draft','submitted','management_approved','returned','finance_confirmed'
                    ) THEN 1 ELSE NULL END
                ) STORED,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                UNIQUE KEY uq_compensation_proposal_open (employment_id, phase, open_slot),
                INDEX idx_cp_status (status, created_at),
                INDEX idx_cp_assignee (assignee_user_id, status),
                INDEX idx_cp_scope_status (branch_id, department_id, status),
                INDEX idx_cp_employment (employment_id, phase, status),
                CONSTRAINT fk_cp_employment FOREIGN KEY (employment_id) REFERENCES employment(id) ON DELETE CASCADE,
                CONSTRAINT fk_cp_branch FOREIGN KEY (branch_id) REFERENCES branches(id),
                CONSTRAINT fk_cp_department FOREIGN KEY (department_id) REFERENCES departments(id),
                CONSTRAINT fk_cp_assignee FOREIGN KEY (assignee_user_id) REFERENCES users(id),
                CONSTRAINT fk_cp_proposed_by FOREIGN KEY (proposed_by) REFERENCES users(id),
                CONSTRAINT fk_cp_submitted_by FOREIGN KEY (submitted_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT fk_cp_approved_by FOREIGN KEY (approved_by) REFERENCES users(id),
                CONSTRAINT fk_cp_finance_by FOREIGN KEY (finance_confirmed_by) REFERENCES users(id),
                CONSTRAINT fk_cp_salary_profile FOREIGN KEY (salary_profile_id) REFERENCES salaryProfile(id) ON DELETE SET NULL,
                CONSTRAINT chk_cp_pay_basis CHECK (
                    (pay_type = 'monthly' AND base_salary IS NOT NULL AND base_salary >= 0 AND hourly_rate IS NULL) OR
                    (pay_type = 'hourly' AND hourly_rate IS NOT NULL AND hourly_rate >= 0 AND base_salary IS NULL)
                ),
                CONSTRAINT chk_cp_social_insurance CHECK (social_insurance_housing_fund >= 0),
                CONSTRAINT chk_cp_row_version CHECK (row_version > 0),
                CONSTRAINT chk_cp_sod_propose_approve CHECK (
                    approved_by IS NULL OR proposed_by IS NULL OR approved_by <> proposed_by
                ),
                CONSTRAINT chk_cp_sod_approve_finance CHECK (
                    finance_confirmed_by IS NULL OR approved_by IS NULL OR finance_confirmed_by <> approved_by
                ),
                CONSTRAINT chk_cp_sod_propose_finance CHECK (
                    finance_confirmed_by IS NULL OR proposed_by IS NULL OR finance_confirmed_by <> proposed_by
                )
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &session) {
                // 增量对齐 open_slot：v6 开放案件含 finance_confirmed，避免遗留行绕过唯一约束。
                // 安全/并发不变量：失败必须抛出阻断启动，禁止 catch-and-skip 永久丢掉唯一性保护。
                mysqlx::Row exprRow =
                    session
                        .sql("SELECT GENERATION_EXPRESSION FROM information_schema.COLUMNS "
                             "WHERE TABLE_SCHEMA = DATABASE() "
                             "AND TABLE_NAME = 'compensation_proposal' "
                             "AND COLUMN_NAME = 'open_slot' LIMIT 1")
                        .execute()
                        .fetchOne();
                if (!exprRow || exprRow[0].isNull())
                {
                    return;
                }
                const std::string expr = exprRow[0].get<std::string>();
                if (expr.find("finance_confirmed") != std::string::npos)
                {
                    return;
                }

                // 预检：新 open-status 集合下 (employment_id, phase) 不得有多条开放案件。
                // 若存在重复，原子 ALTER 加唯一索引也会失败；此处显式 fail-closed 给出可操作错误。
                mysqlx::Row dupRow =
                    session
                        .sql(R"SQL(
                            SELECT employment_id, phase, COUNT(*) AS cnt
                            FROM compensation_proposal
                            WHERE status IN (
                                'draft','submitted','management_approved','returned','finance_confirmed'
                            )
                            GROUP BY employment_id, phase
                            HAVING cnt > 1
                            LIMIT 1
                        )SQL")
                        .execute()
                        .fetchOne();
                if (dupRow)
                {
                    throw std::runtime_error(
                        "compensation_proposal.open_slot align aborted: duplicate open "
                        "(employment_id, phase) rows under v6 open-status set; resolve before upgrade");
                }

                // 单条原子 ALTER：DROP/ADD 同语句，避免先丢唯一索引再失败留下 fail-open 窗口。
                // 分 index-present / index-absent 两条路径，各自仍是一条 ALTER。
                const bool hasOpenIndex = Common::indexExists(
                    dbManager, "compensation_proposal", "uq_compensation_proposal_open");
                if (hasOpenIndex)
                {
                    session
                        .sql(R"SQL(
                            ALTER TABLE compensation_proposal
                            DROP INDEX uq_compensation_proposal_open,
                            DROP COLUMN open_slot,
                            ADD COLUMN open_slot TINYINT GENERATED ALWAYS AS (
                                CASE WHEN status IN (
                                    'draft','submitted','management_approved','returned','finance_confirmed'
                                ) THEN 1 ELSE NULL END
                            ) STORED,
                            ADD UNIQUE KEY uq_compensation_proposal_open
                                (employment_id, phase, open_slot)
                        )SQL")
                        .execute();
                }
                else
                {
                    session
                        .sql(R"SQL(
                            ALTER TABLE compensation_proposal
                            DROP COLUMN open_slot,
                            ADD COLUMN open_slot TINYINT GENERATED ALWAYS AS (
                                CASE WHEN status IN (
                                    'draft','submitted','management_approved','returned','finance_confirmed'
                                ) THEN 1 ELSE NULL END
                            ) STORED,
                            ADD UNIQUE KEY uq_compensation_proposal_open
                                (employment_id, phase, open_slot)
                        )SQL")
                        .execute();
                }
                std::cout << "compensation_proposal.open_slot aligned to include finance_confirmed."
                          << std::endl;
            },
        },
        {
            "payrollPeriod",
            R"SQL(CREATE TABLE payrollPeriod (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                payroll_month DATE NOT NULL COMMENT '工资月份，固定保存为当月第一天',
                status ENUM('calculating', 'first_review', 'submitted_for_supervisor', 'second_review', 'correction_required', 'locked', 'archived') NOT NULL DEFAULT 'calculating',
                version_no INT NOT NULL DEFAULT 1 COMMENT '同一月份的工资版本号（业务修订版）',
                row_version INT NOT NULL DEFAULT 1 COMMENT '乐观并发版本，状态/审核信息变更时递增',
                first_reviewed_by INT NULL,
                first_reviewed_at DATETIME NULL,
                reviewed_by INT NULL COMMENT '旧字段：兼容读取，新流程写 submitted_* / supervisor_*',
                reviewed_at DATETIME NULL,
                submitted_by INT NULL,
                submitted_at DATETIME NULL,
                supervisor_reviewed_by INT NULL,
                supervisor_reviewed_at DATETIME NULL,
                supervisor_decision ENUM('approve', 'return') NULL,
                supervisor_note VARCHAR(1000) NOT NULL DEFAULT '',
                review_note VARCHAR(1000) NOT NULL DEFAULT '' COMMENT '财务提交说明',
                revision_of_period_id INT NULL,
                locked_by INT NULL,
                locked_at DATETIME NULL,
                total_salary DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '该工资周期实发总额',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_payrollPeriod_first_reviewed_by FOREIGN KEY (first_reviewed_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT fk_payrollPeriod_reviewed_by FOREIGN KEY (reviewed_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT fk_payrollPeriod_submitted_by FOREIGN KEY (submitted_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT fk_payrollPeriod_supervisor_reviewed_by FOREIGN KEY (supervisor_reviewed_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT fk_payrollPeriod_locked_by FOREIGN KEY (locked_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT fk_payrollPeriod_revision_of FOREIGN KEY (revision_of_period_id) REFERENCES payrollPeriod(id) ON DELETE SET NULL,
                CONSTRAINT chk_payrollPeriod_month_first_day CHECK (DAY(payroll_month) = 1),
                CONSTRAINT chk_payrollPeriod_version CHECK (version_no > 0),
                CONSTRAINT chk_payrollPeriod_row_version CHECK (row_version > 0),
                UNIQUE KEY uq_payrollPeriod_month_version (payroll_month, version_no),
                INDEX idx_payrollPeriod_status_month (status, payroll_month)
            ))SQL",
            migratePayrollPeriodColumns,
            migratePayrollPeriodColumns,
        },
        {
            "salary",
            R"SQL(CREATE TABLE salary (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                payroll_period_id INT NOT NULL,
                salary_profile_id INT NULL COMMENT '生成工资时采用的薪资配置',
                user_id INT NOT NULL,
                pay_type ENUM('monthly', 'hourly') NOT NULL COMMENT '当月计薪方式快照',
                base_salary DECIMAL(18, 2) NULL COMMENT '当月月薪标准快照',
                hourly_rate DECIMAL(18, 2) NULL COMMENT '当月时薪标准快照',
                work_hours_month DECIMAL(10, 2) NOT NULL DEFAULT 0.00 COMMENT '本月工作时间（小时）',
                attendance_award DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '全勤奖金',
                performance_award DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '绩效奖金',
                allowance DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '补贴',
                deduction DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '财务依据罚款凭证录入的扣款',
                social_insurance_housing_fund DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '五险一金扣款金额',
                total_salary DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '当月实发工资',
                review_status ENUM('pending', 'first_reviewed', 'returned', 'second_reviewed', 'locked') NOT NULL DEFAULT 'pending',
                first_reviewed_by INT NULL,
                first_reviewed_at DATETIME NULL,
                review_note VARCHAR(1000) NOT NULL DEFAULT '',
                is_manually_modified TINYINT NOT NULL DEFAULT 0 COMMENT '是否存在人工修改',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                CONSTRAINT fk_salary_payroll_period FOREIGN KEY (payroll_period_id) REFERENCES payrollPeriod(id) ON DELETE CASCADE,
                CONSTRAINT fk_salary_profile FOREIGN KEY (salary_profile_id) REFERENCES salaryProfile(id) ON DELETE SET NULL,
                CONSTRAINT fk_salary_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE RESTRICT,
                CONSTRAINT fk_salary_first_reviewed_by FOREIGN KEY (first_reviewed_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT chk_salary_pay_basis CHECK (
                    (pay_type = 'monthly' AND base_salary IS NOT NULL AND base_salary >= 0 AND hourly_rate IS NULL) OR
                    (pay_type = 'hourly' AND hourly_rate IS NOT NULL AND hourly_rate >= 0 AND base_salary IS NULL)
                ),
                CONSTRAINT chk_salary_non_negative CHECK (
                    work_hours_month >= 0 AND attendance_award >= 0 AND performance_award >= 0 AND allowance >= 0 AND
                    deduction >= 0 AND social_insurance_housing_fund >= 0
                ),
                UNIQUE KEY uq_salary_period_user (payroll_period_id, user_id),
                INDEX idx_salary_user_period (user_id, payroll_period_id),
                INDEX idx_salary_review_status (payroll_period_id, review_status),
                INDEX idx_salary_manually_modified (payroll_period_id, is_manually_modified)
            ))SQL",
            migratePayrollPeriodColumns,
            migratePayrollPeriodColumns,
        },
        {
            "payrollPeriodAuditEvent",
            R"SQL(CREATE TABLE payrollPeriodAuditEvent (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                period_id INT NOT NULL,
                version_no INT NOT NULL DEFAULT 1,
                before_row_version INT NULL,
                after_row_version INT NULL,
                action VARCHAR(64) NOT NULL,
                decision VARCHAR(32) NULL,
                operator_id INT NULL,
                operator_department_id INT NULL,
                before_status VARCHAR(64) NULL,
                after_status VARCHAR(64) NULL,
                note VARCHAR(1000) NOT NULL DEFAULT '',
                request_id VARCHAR(64) NOT NULL DEFAULT '',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                INDEX idx_payroll_audit_period_created (period_id, created_at),
                INDEX idx_payroll_audit_operator (operator_id, created_at),
                CONSTRAINT fk_payroll_audit_period FOREIGN KEY (period_id) REFERENCES payrollPeriod(id) ON DELETE CASCADE,
                CONSTRAINT fk_payroll_audit_operator FOREIGN KEY (operator_id) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            migratePayrollPeriodColumns,
        },
        {
            "salaryChangeRecord",
            R"SQL(CREATE TABLE salaryChangeRecord (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                salary_id BIGINT NOT NULL,
                changed_field VARCHAR(64) NOT NULL COMMENT '被修改的工资字段',
                before_value VARCHAR(255) NULL COMMENT '修改前值',
                after_value VARCHAR(255) NULL COMMENT '修改后值',
                changed_by INT NULL,
                change_reason VARCHAR(500) NOT NULL DEFAULT '' COMMENT '人工修改原因或凭证说明',
                evidence_path VARCHAR(500) NOT NULL DEFAULT '' COMMENT '相关凭证文件路径',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                CONSTRAINT fk_salaryChangeRecord_salary FOREIGN KEY (salary_id) REFERENCES salary(id) ON DELETE CASCADE,
                CONSTRAINT fk_salaryChangeRecord_changed_by FOREIGN KEY (changed_by) REFERENCES users(id) ON DELETE SET NULL,
                INDEX idx_salaryChangeRecord_salary_created (salary_id, created_at),
                INDEX idx_salaryChangeRecord_changed_by (changed_by, created_at)
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
            "monthlyFinancialRecord",
            R"SQL(CREATE TABLE monthlyFinancialRecord (
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
                last_attendance_event_at DATETIME NULL,
                CONSTRAINT fk_doctor_user FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                ForeignKeys::migrateOnlineDoctors(dbManager);
                // 考勤联动的旧事件防回写守卫依赖此列；旧库必须补，否则打卡联动 SQL 直接报错。
                Common::addColumnIfNotExists(dbManager, "onlineDoctors", "last_attendance_event_at", "DATETIME NULL");
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
                department_id INT NULL COMMENT '开单医生所属部门快照，用于财务按部门/分院归集营收',
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
                CONSTRAINT fk_orders_department_id FOREIGN KEY (department_id) REFERENCES departments(id) ON DELETE SET NULL,
                INDEX idx_orders_owner_deleted (owner_id, is_deleted),
                INDEX idx_orders_doctor_time (doctor_id, created_at),
                INDEX idx_orders_department (department_id),
                INDEX idx_petId_time (pet_id, created_at)
            ))SQL",
            nullptr,
            [](DatabaseManagerInterface &dbManager, mysqlx::Session &)
            {
                // 先补列（含必需的 department_id），再补外键：fk_orders_department_id 依赖该列先存在。
                Columns::migrateOrders(dbManager);
                ForeignKeys::migrateOrders(dbManager);
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
            "report_templates",
            R"SQL(CREATE TABLE report_templates (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                code VARCHAR(64) NOT NULL,
                name VARCHAR(128) NOT NULL,
                document_type VARCHAR(64) NOT NULL,
                paper_size VARCHAR(16) NOT NULL DEFAULT 'A4',
                orientation ENUM('portrait', 'landscape') NOT NULL DEFAULT 'portrait',
                status ENUM('draft', 'published', 'archived') NOT NULL DEFAULT 'draft',
                current_version_id INT NULL,
                created_by INT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                UNIQUE INDEX uq_report_templates_code (code),
                INDEX idx_report_templates_type_status (document_type, status),
                INDEX idx_report_templates_current_version (current_version_id),
                CONSTRAINT fk_report_templates_created_by FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            seedDefaultReportTemplate,
            seedDefaultReportTemplate,
        },
        {
            "report_template_versions",
            R"SQL(CREATE TABLE report_template_versions (
                id INT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                template_id INT NOT NULL,
                version_no INT NOT NULL,
                engine VARCHAR(32) NOT NULL DEFAULT 'html',
                template_content LONGTEXT NOT NULL,
                data_contract_version VARCHAR(64) NOT NULL DEFAULT 'medical-document.v1',
                status ENUM('draft', 'published', 'archived') NOT NULL DEFAULT 'draft',
                created_by INT NULL,
                published_by INT NULL,
                published_at DATETIME NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                UNIQUE INDEX uq_report_template_version (template_id, version_no),
                INDEX idx_report_template_versions_status (template_id, status),
                CONSTRAINT fk_report_template_versions_template FOREIGN KEY (template_id) REFERENCES report_templates(id) ON DELETE RESTRICT,
                CONSTRAINT fk_report_template_versions_created_by FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT fk_report_template_versions_published_by FOREIGN KEY (published_by) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            ensureReportTemplateCurrentVersionForeignKey,
            ensureReportTemplateCurrentVersionForeignKey,
        },
        {
            "medical_documents",
            R"SQL(CREATE TABLE medical_documents (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                document_no VARCHAR(48) NOT NULL,
                order_id INT NOT NULL,
                owner_id INT NOT NULL,
                pet_id INT NOT NULL,
                doctor_id INT NOT NULL,
                status ENUM('draft', 'finalized', 'amended', 'voided') NOT NULL DEFAULT 'draft',
                chief_complaint TEXT NOT NULL,
                present_illness TEXT NOT NULL,
                past_history TEXT NOT NULL,
                allergies TEXT NOT NULL,
                physical_exam TEXT NOT NULL,
                diagnosis TEXT NOT NULL,
                treatment_plan TEXT NOT NULL,
                discharge_advice TEXT NOT NULL,
                follow_up_at DATETIME NULL,
                structured_data JSON NULL,
                revision_no INT NOT NULL DEFAULT 0,
                lock_version INT NOT NULL DEFAULT 0,
                template_version_id INT NULL,
                finalized_by INT NULL,
                finalized_at DATETIME NULL,
                voided_by INT NULL,
                voided_at DATETIME NULL,
                void_reason VARCHAR(500) NOT NULL DEFAULT '',
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                UNIQUE INDEX uq_medical_documents_no (document_no),
                UNIQUE INDEX uq_medical_documents_order (order_id),
                INDEX idx_medical_documents_doctor_status_time (doctor_id, status, created_at),
                INDEX idx_medical_documents_owner_time (owner_id, created_at),
                INDEX idx_medical_documents_pet_time (pet_id, created_at),
                CONSTRAINT fk_medical_documents_order FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE RESTRICT,
                CONSTRAINT fk_medical_documents_owner FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE RESTRICT,
                CONSTRAINT fk_medical_documents_pet FOREIGN KEY (pet_id) REFERENCES pets(id) ON DELETE RESTRICT,
                CONSTRAINT fk_medical_documents_doctor FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE RESTRICT,
                CONSTRAINT fk_medical_documents_template_version FOREIGN KEY (template_version_id) REFERENCES report_template_versions(id) ON DELETE RESTRICT,
                CONSTRAINT fk_medical_documents_finalized_by FOREIGN KEY (finalized_by) REFERENCES users(id) ON DELETE SET NULL,
                CONSTRAINT fk_medical_documents_voided_by FOREIGN KEY (voided_by) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
        },
        {
            "medical_prescription_items",
            R"SQL(CREATE TABLE medical_prescription_items (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                medical_document_id BIGINT NOT NULL,
                medicine_id INT NULL,
                medicine_name VARCHAR(255) NOT NULL,
                specification VARCHAR(255) NOT NULL DEFAULT '',
                unit VARCHAR(32) NOT NULL DEFAULT '',
                dosage VARCHAR(128) NOT NULL DEFAULT '',
                frequency VARCHAR(128) NOT NULL DEFAULT '',
                route VARCHAR(128) NOT NULL DEFAULT '',
                duration_days INT NOT NULL DEFAULT 0,
                quantity INT NOT NULL DEFAULT 0,
                instructions VARCHAR(500) NOT NULL DEFAULT '',
                unit_price DECIMAL(18,2) NOT NULL DEFAULT 0.00,
                total_price DECIMAL(18,2) NOT NULL DEFAULT 0.00,
                sort_order INT NOT NULL DEFAULT 0,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                INDEX idx_medical_prescription_document (medical_document_id, sort_order),
                INDEX idx_medical_prescription_medicine (medicine_id),
                CONSTRAINT fk_medical_prescription_document FOREIGN KEY (medical_document_id) REFERENCES medical_documents(id) ON DELETE CASCADE,
                CONSTRAINT fk_medical_prescription_medicine FOREIGN KEY (medicine_id) REFERENCES warehouse(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
        },
        {
            "medical_document_versions",
            R"SQL(CREATE TABLE medical_document_versions (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                medical_document_id BIGINT NOT NULL,
                revision_no INT NOT NULL,
                snapshot_json JSON NOT NULL,
                change_reason VARCHAR(500) NOT NULL DEFAULT '',
                content_hash CHAR(64) NOT NULL,
                created_by INT NULL,
                created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                UNIQUE INDEX uq_medical_document_revision (medical_document_id, revision_no),
                INDEX idx_medical_document_versions_hash (content_hash),
                CONSTRAINT fk_medical_document_versions_document FOREIGN KEY (medical_document_id) REFERENCES medical_documents(id) ON DELETE RESTRICT,
                CONSTRAINT fk_medical_document_versions_created_by FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
        },
        {
            "report_artifacts",
            R"SQL(CREATE TABLE report_artifacts (
                id BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
                medical_document_version_id BIGINT NOT NULL,
                template_version_id INT NOT NULL,
                format VARCHAR(16) NOT NULL DEFAULT 'pdf',
                storage_key VARCHAR(500) NOT NULL,
                sha256 CHAR(64) NOT NULL,
                payload_hash CHAR(64) NOT NULL,
                byte_size BIGINT NOT NULL DEFAULT 0,
                page_count INT NULL,
                generated_by INT NULL,
                generated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                UNIQUE INDEX uq_report_artifact_version_template_format (medical_document_version_id, template_version_id, format),
                INDEX idx_report_artifacts_sha256 (sha256),
                CONSTRAINT fk_report_artifacts_document_version FOREIGN KEY (medical_document_version_id) REFERENCES medical_document_versions(id) ON DELETE RESTRICT,
                CONSTRAINT fk_report_artifacts_template_version FOREIGN KEY (template_version_id) REFERENCES report_template_versions(id) ON DELETE RESTRICT,
                CONSTRAINT fk_report_artifacts_generated_by FOREIGN KEY (generated_by) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4)SQL",
            nullptr,
            nullptr,
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
            existing.insert(normalizeTableName(table.getName()));
        }

        for (const TableSpec &spec : kTables)
        {
            if (existing.count(normalizeTableName(spec.name)))
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
