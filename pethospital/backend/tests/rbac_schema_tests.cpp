#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef DATABASE_MIGRATIONS_SOURCE_PATH
#error "DATABASE_MIGRATIONS_SOURCE_PATH is required"
#endif

namespace
{
std::string readFile(const char *path)
{
    std::ifstream file(path);
    assert(file.is_open());
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void assertContains(const std::string &source, const std::string &needle)
{
    assert(source.find(needle) != std::string::npos);
}

void assertNotContains(const std::string &source, const std::string &needle)
{
    assert(source.find(needle) == std::string::npos);
}
}

int main()
{
    const std::string migrations = readFile(DATABASE_MIGRATIONS_SOURCE_PATH);

    assertNotContains(migrations, "\"types\"");
    assertNotContains(migrations, "CREATE TABLE types");
    assertNotContains(migrations, "type_id INT NOT NULL");
    assertNotContains(migrations, "CONSTRAINT fk_user_type");

    assertContains(migrations, "\"branches\"");
    assertContains(migrations, "CREATE TABLE branches");
    assertContains(migrations, "system_key VARCHAR(32) NULL UNIQUE");
    assertContains(migrations, "INSERT INTO branches");
    assertContains(migrations, "'总院'");

    assertContains(migrations, "\"departments\"");
    assertContains(migrations, "CREATE TABLE departments");
    assertContains(migrations, "branch_id INT NOT NULL");
    assertContains(migrations, "description VARCHAR(255) NOT NULL DEFAULT ''");
    assertContains(migrations, "CONSTRAINT fk_department_branch FOREIGN KEY (branch_id) REFERENCES branches(id)");
    assertContains(migrations, "migrateDepartmentOrgColumns");
    assertContains(migrations, "system_key VARCHAR(32) NULL UNIQUE");
    assertContains(migrations, "INSERT INTO departments");
    assertContains(migrations, "'医疗部', 'medical'");
    assertContains(migrations, "'管理部' AS name, 'management'");

    assertContains(migrations, "\"positions\"");
    assertContains(migrations, "CREATE TABLE positions");
    assertContains(migrations, "migratePositionDescriptionColumn");
    assertContains(migrations, "staff_kind ENUM('doctor','nurse','warehouse','finance','management','personnel','general_staff')");
    assertContains(migrations, "system_key VARCHAR(32) NULL UNIQUE");
    assertContains(migrations, "CONSTRAINT fk_position_dept FOREIGN KEY (department_id) REFERENCES departments(id)");
    assertContains(migrations, "INSERT INTO positions");
    assertContains(migrations, "'super-admin'");
    assertContains(migrations, "'doctor'");
    assertContains(migrations, "'nurse'");

    assertContains(migrations, "\"position_permissions\"");
    assertContains(migrations, "CREATE TABLE position_permissions");
    assertContains(migrations, "PRIMARY KEY (position_id, permission_key)");
    assertContains(migrations, "CONSTRAINT chk_position_permission_not_meta CHECK (permission_key <> 'rbac:manage')");
    assertContains(migrations, "INSERT INTO position_permissions");
    assertContains(migrations, "'portal:boss'");
    assertContains(migrations, "'salary:read'");
    assertContains(migrations, "'medical-record:write'");
    assertContains(migrations, "'stock:write'");
    assertContains(migrations, "'attendance:read'");
    assertContains(migrations, "'attendance:manage'");
    assertNotContains(migrations, "('super-admin', 'rbac:manage')");

    assertContains(migrations, "\"user_permissions\"");
    assertContains(migrations, "CREATE TABLE user_permissions");
    assertContains(migrations, "UNIQUE KEY uq_user_permission (user_id, permission_key)");
    assertContains(migrations, "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE");
    assertContains(migrations, "FOREIGN KEY (granted_by) REFERENCES users(id) ON DELETE SET NULL");

    assertContains(migrations, "\"permission_templates\"");
    assertContains(migrations, "\"permission_template_items\"");
    assertContains(migrations, "CONSTRAINT chk_template_permission_not_meta CHECK (permission_key <> 'rbac:manage')");
    assertContains(migrations, "INSERT INTO permission_templates");
    assertContains(migrations, "INSERT INTO permission_template_items");

    assertContains(migrations, "\"user_scopes\"");
    assertContains(migrations, "CREATE TABLE user_scopes");
    assertContains(migrations, "branch_id INT NULL");
    assertContains(migrations, "department_id INT NULL");
    assertContains(migrations, "CONSTRAINT fk_user_scope_branch FOREIGN KEY (branch_id) REFERENCES branches(id)");
    assertContains(migrations, "CONSTRAINT fk_user_scope_department FOREIGN KEY (department_id) REFERENCES departments(id)");
    assertContains(migrations, "CONSTRAINT chk_user_scope_level CHECK (branch_id IS NOT NULL OR department_id IS NOT NULL)");

    assertContains(migrations, "\"users\"");
    assertContains(migrations, "account_type ENUM('customer','staff') NOT NULL DEFAULT 'customer'");
    assertContains(migrations, "position_id INT NULL");
    assertContains(migrations, "attendance_no VARCHAR(32) NULL");
    assertContains(migrations, "UNIQUE KEY uq_users_attendance_no (attendance_no)");
    assertContains(migrations, "CONSTRAINT fk_users_position FOREIGN KEY (position_id) REFERENCES positions(id)");
    assertContains(migrations, "CONSTRAINT chk_account_position CHECK");
    assertContains(migrations, "account_type = 'customer' AND position_id IS NULL");
    assertContains(migrations, "account_type = 'staff' AND position_id IS NOT NULL");
    assertContains(migrations, "ensureBootstrapSuperAdmin");
    assertContains(migrations, "PETMANAGER_BOOTSTRAP_ADMIN_PASSWORD");
    assertContains(migrations, "SELECT id FROM positions WHERE system_key = 'super-admin' LIMIT 1");
    assertContains(migrations, "VALUES ('staff', ?, '系统管理员', ?, ?)");
    assertContains(migrations, "hash_password(password)");
    assertNotContains(migrations, "SHA2(?, 256)");
    assertNotContains(migrations, "password123");
    assertNotContains(migrations, "admin123");

    assertContains(migrations, "operator_department_id INT NULL");
    assertContains(migrations, "CONSTRAINT fk_user_operations_operator_department");
    assertContains(migrations, "last_attendance_event_at DATETIME NULL");

    assertContains(migrations, "\"attendance_devices\"");
    assertContains(migrations, "\"attendance_punches\"");
    assertContains(migrations, "\"attendance_records\"");
    assertContains(migrations, "\"attendance_device_nonces\"");
    assertContains(migrations, "UNIQUE KEY uq_attendance_punch_event (device_id, event_id)");
    assertContains(migrations, "UNIQUE KEY uq_attendance_records_user_date (user_id, work_date)");
    assertContains(migrations, "PRIMARY KEY (device_id, nonce)");

    return 0;
}
