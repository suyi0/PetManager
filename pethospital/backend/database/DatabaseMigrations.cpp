#include "DatabaseMigrations.h"

#include "migrations/backfills/RelationBackfills.h"
#include "migrations/columns/ColumnMigrations.h"
#include "migrations/foreign_keys/ForeignKeyMigrations.h"

#include <exception>
#include <iostream>
#include <mutex>
#include <string>

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

        try
        {
            session->sql("SET SESSION lock_wait_timeout = 10").execute();
        }
        catch (const std::exception &e)
        {
            std::cout << "Failed to set lock_wait_timeout: " << e.what() << std::endl;
        }

        bool types_exists = false;
        bool phones_exists = false;
        bool users_exists = false;
        bool address_exists = false;
        bool salary_exists = false;
        bool salaryRecord_exists = false;
        bool monthlySalaryRecord_exists = false;
        bool stock_exists = false;
        bool workTimes_exists = false;
        bool onlineDoctors_exists = false;
        bool reservates_exists = false;
        bool orders_exists = false;
        bool orderMedicines_exists = false;
        bool pets_exists = false;
        bool userSearch_exists = false;
        bool warehouse_exists = false;
        bool workTimeRecords_exists = false;
        bool system_operations_exists = false;
        bool user_operations_exists = false;

        for (const auto &table : schema->getTables())
        {
            const std::string table_name = table.getName();
            if (table_name == "types")
            {
                types_exists = true;
            }
            else if (table_name == "phones")
            {
                phones_exists = true;
            }
            else if (table_name == "users")
            {
                users_exists = true;
            }
            else if (table_name == "address")
            {
                address_exists = true;
            }
            else if (table_name == "salary")
            {
                salary_exists = true;
            }
            else if (table_name == "salaryRecord")
            {
                salaryRecord_exists = true;
            }
            else if (table_name == "monthlySalaryRecord")
            {
                monthlySalaryRecord_exists = true;
            }
            else if (table_name == "stock")
            {
                stock_exists = true;
            }
            else if (table_name == "workTimes")
            {
                workTimes_exists = true;
            }
            else if (table_name == "onlineDoctors")
            {
                onlineDoctors_exists = true;
            }
            else if (table_name == "reaservations")
            {
                reservates_exists = true;
            }
            else if (table_name == "orders")
            {
                orders_exists = true;
            }
            else if (table_name == "orderMedicines")
            {
                orderMedicines_exists = true;
            }
            else if (table_name == "pets")
            {
                pets_exists = true;
            }
            else if (table_name == "userSearch")
            {
                userSearch_exists = true;
            }
            else if (table_name == "warehouse")
            {
                warehouse_exists = true;
            }
            else if (table_name == "workTimeRecords")
            {
                workTimeRecords_exists = true;
            }
            else if (table_name == "system_operations")
            {
                system_operations_exists = true;
            }
            else if (table_name == "user_operations")
            {
                user_operations_exists = true;
            }
        }

        if (types_exists)
        {
            std::cout << "Types table is exists." << std::endl;
        }
        else
        {
            std::cout << "types table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE types ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "type VARCHAR(255)"
                         ")")
                .execute();
            session->sql("INSERT INTO types (type) VALUES ('总裁'), ('副总裁'), ('财务经理'), ('人事经理'), ('部门经理'), ('超级管理员'), ('仓库管理员'), ('医生'), ('护士'), ('普通用户')").execute();
            std::cout << "types table created successfully." << std::endl;
        }

        if (salaryRecord_exists)
        {
            std::cout << "salaryRecord table is exists." << std::endl;
        }
        else
        {
            std::cout << "salaryRecord table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE salaryRecord ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "salesCount DECIMAL(18, 2) COMMENT '销售金额', "
                         "costCount DECIMAL(18, 2) COMMENT '成本金额', "
                         "profitCount DECIMAL(18, 2) COMMENT '利润金额', "
                         "record_type ENUM('day', 'month'), "
                         "is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除', "
                         "deleted_at DATETIME NULL COMMENT '软删除时间', "
                         "deleted_by INT NULL COMMENT '执行删除的用户ID', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "INDEX idx_salaryRecord_is_deleted (is_deleted) "
                         ")")
                .execute();
            std::cout << "salaryRecord table created successfully." << std::endl;
        }

        if (users_exists)
        {
            std::cout << "users table is exists." << std::endl;
            Columns::migrateUsers(dbManager);
            ForeignKeys::migrateUsers(dbManager);
        }
        else
        {
            std::cout << "users table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE users ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "type_id INT NOT NULL, "
                         "name VARCHAR(255), "
                         "password VARCHAR(255), "
                         "phone VARCHAR(20), "
                         "email VARCHAR(255), "
                         "birthday DATE, "
                         "head_image VARCHAR(255),"
                         "user_specialty VARCHAR(255),"
                         "user_introduction TEXT, "
                         "user_level int, "
                         "funds DECIMAL(18, 2) NOT NULL DEFAULT 0.00 COMMENT '用户账户余额', "
                         "is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除', "
                         "deleted_at DATETIME NULL COMMENT '软删除时间', "
                         "deleted_by INT NULL COMMENT '执行删除的用户ID', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "CONSTRAINT fk_user_type FOREIGN KEY (type_id) REFERENCES types(id) ON DELETE CASCADE, "
                         "INDEX idx_users_name (name), "
                         "INDEX idx_users_is_deleted (is_deleted) "
                         ")")
                .execute();
            std::cout << "users table created successfully." << std::endl;
        }

        if (address_exists)
        {
            std::cout << "address table is exists." << std::endl;
        }
        else
        {
            std::cout << "address table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE address ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "user_id INT NOT NULL COMMENT '地址所属用户ID', "
                         "contact_name VARCHAR(80) COMMENT '联系人姓名', "
                         "contact_phone VARCHAR(20) COMMENT '联系人手机号', "
                         "country VARCHAR(80) DEFAULT '中国' COMMENT '国家或地区', "
                         "province VARCHAR(80) COMMENT '省/直辖市/自治区', "
                         "city VARCHAR(80) COMMENT '城市', "
                         "district VARCHAR(80) COMMENT '区/县', "
                         "detail_address VARCHAR(255) COMMENT '门牌号及详细地址', "
                         "address_text VARCHAR(500) COMMENT '完整地址文本', "
                         "postal_code VARCHAR(20) COMMENT '邮政编码，用户选填或后端根据地址库补全', "
                         "address_tag ENUM('家', '公司', '医院', '学校', '其他') DEFAULT '家' COMMENT '地址标签', "
                         "is_default TINYINT NOT NULL DEFAULT 0 COMMENT '是否默认地址', "
                         "longitude DECIMAL(10, 7) DEFAULT 0.0000000 COMMENT '经度', "
                         "latitude DECIMAL(10, 7) DEFAULT 0.0000000 COMMENT '纬度', "
                         "geocode_source VARCHAR(50) COMMENT '地理编码来源', "
                         "remarks VARCHAR(255) COMMENT '备注', "
                         "is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除', "
                         "deleted_at DATETIME NULL COMMENT '软删除时间', "
                         "deleted_by INT NULL COMMENT '执行删除的用户ID', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "CONSTRAINT fk_address_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
                         "INDEX idx_address_user_default (user_id, is_default, is_deleted), "
                         "INDEX idx_address_contact_phone (contact_phone), "
                         "INDEX idx_address_region (province, city, district), "
                         "INDEX idx_address_is_deleted (is_deleted) "
                         ")")
                .execute();
            std::cout << "address table created successfully." << std::endl;
        }

        if (salary_exists)
        {
            std::cout << "salary table is exists." << std::endl;
        }
        else
        {
            std::cout << "salary table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE salary ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "user_id INT NOT NULL, "
                         "base_salary DECIMAL(18, 2) COMMENT '基本工资', "
                         "PA_Award DECIMAL(18, 2) COMMENT '全勤奖', "
                         "PB_Award DECIMAL(18, 2) COMMENT '绩效奖金', "
                         "total_salary DECIMAL(18, 2) COMMENT '总工资', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "CONSTRAINT fk_salary_userId FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
                         "UNIQUE KEY uq_salary_user_id (user_id) "
                         ")")
                .execute();
            std::cout << "salary table created successfully." << std::endl;
        }

        if (phones_exists)
        {
            std::cout << "phones table is exists." << std::endl;
            ForeignKeys::migratePhones(dbManager);
            Columns::migratePhones(dbManager);
            Backfills::backfillPhones(dbManager);
        }
        else
        {
            std::cout << "phones table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE phones ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "user_id INT NOT NULL, "
                         "phone VARCHAR(20), "
                         "phone_lastfour VARCHAR(4) GENERATED ALWAYS AS (SUBSTRING(phone, -4)), "
                         "CONSTRAINT fk_phones_users FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
                         "INDEX idx_phone_exp (phone), "
                         "INDEX idx_phone_lastfour (phone_lastfour)"
                         ")")
                .execute();
            std::cout << "phones table created successfully." << std::endl;
            Backfills::backfillPhones(dbManager);
        }

        if (pets_exists)
        {
            std::cout << "pets table is exists." << std::endl;
            Columns::migratePets(dbManager);
            ForeignKeys::migratePets(dbManager);
        }
        else
        {
            std::cout << "pets table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE pets ( "
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "user_id INT NOT NULL, "
                         "pet_name VARCHAR(255), "
                         "pet_type VARCHAR(255), "
                         "pet_age VARCHAR(255), "
                         "pet_sex VARCHAR(255), "
                         "pet_breed VARCHAR(255), "
                         "pet_neutered VARCHAR(255), "
                         "vaccine_status VARCHAR(255), "
                         "preference TEXT, "
                         "notes TEXT, "
                         "is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除', "
                         "deleted_at DATETIME NULL COMMENT '软删除时间', "
                         "deleted_by INT NULL COMMENT '执行删除的用户ID', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "CONSTRAINT fk_pets_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
                         "INDEX idx_pets_is_deleted (is_deleted) "
                         ")")
                .execute();
            std::cout << "pets table created successfully" << std::endl;
        }

        if (userSearch_exists)
        {
            std::cout << "userSearch table is exists." << std::endl;
        }
        else
        {
            std::cout << "userSearch table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE userSearch ( "
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "user_id INT NOT NULL, "
                         "search_text VARCHAR(255) NOT NULL, "
                         "is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除', "
                         "deleted_at DATETIME NULL COMMENT '软删除时间', "
                         "deleted_by INT NULL COMMENT '执行删除的用户ID', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "CONSTRAINT fk_userSearch_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
                         "UNIQUE KEY uk_userSearch_user_text (user_id, search_text), "
                         "INDEX idx_userSearch_recent (user_id, is_deleted, updated_at) "
                         ")")
                .execute();
            std::cout << "userSearch table created successfully." << std::endl;
        }

        if (monthlySalaryRecord_exists)
        {
            std::cout << "monthlySalaryRecord table is exists." << std::endl;
        }
        else
        {
            std::cout << "monthlySalaryRecord table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE monthlySalaryRecord ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "salesCount DECIMAL(18, 2) COMMENT '销售金额', "
                         "costCount DECIMAL(18, 2) COMMENT '成本金额', "
                         "profitCount DECIMAL(18, 2) COMMENT '利润金额', "
                         "business_date DATE NOT NULL, "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP "
                         ")")
                .execute();
            std::cout << "monthlySalaryRecord table created successfully." << std::endl;
        }

        if (stock_exists)
        {
            std::cout << "stock table is exists." << std::endl;
        }
        else
        {
            std::cout << "stock table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE stock ("
                         "id INT AUTO_INCREMENT PRIMARY KEY, "
                         "share_type VARCHAR(255) NOT NULL, "
                         "holder VARCHAR(255) NOT NULL, "
                         "share BIGINT NOT NULL DEFAULT 0, "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "UNIQUE KEY uq_stock_share_type_holder (share_type, holder) "
                         ")")
                .execute();
            std::cout << "stock table created successfully." << std::endl;
        }

        if (warehouse_exists)
        {
            std::cout << "warehouse table is exists." << std::endl;
            Columns::migrateWarehouse(dbManager);
        }
        else
        {
            std::cout << "warehouse table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE warehouse("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "item_name VARCHAR(255), "
                         "item_type VARCHAR(255), "
                         "item_productiondate DATE, "
                         "item_expirationdate DATE, "
                         "days_until_expire INT DEFAULT NULL, "
                         "item_price DECIMAL(10, 2), "
                         "item_number INT, "
                         "item_totalprice DECIMAL(18, 2) GENERATED ALWAYS AS (item_price * item_number) STORED, "
                         "is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除', "
                         "deleted_at DATETIME NULL COMMENT '软删除时间', "
                         "deleted_by INT NULL COMMENT '执行删除的用户ID', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "INDEX idx_id_exp (id, days_until_expire), "
                         "INDEX idx_exp (days_until_expire), "
                         "INDEX idx_warehouse_is_deleted (is_deleted) "
                         ")")
                .execute();
            std::cout << "warehouse table created successfully" << std::endl;
        }

        if (workTimes_exists)
        {
            std::cout << "workTimes table is exists." << std::endl;
        }
        else
        {
            std::cout << "workTimes table is not exists." << std::endl;
            session->sql("CREATE TABLE workTimes ("
                         "check_in_time_start TIME, "
                         "check_in_time_end TIME, "
                         "check_out_time_start TIME,"
                         "check_out_time_end TIME "
                         ")")
                .execute();
            std::cout << "workTimes table created successfully." << std::endl;
        }

        if (onlineDoctors_exists)
        {
            std::cout << "onlineDoctors table is exists." << std::endl;
            ForeignKeys::migrateOnlineDoctors(dbManager);
        }
        else
        {
            std::cout << "onlineDoctors table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE onlineDoctors ( "
                         "doctor_id INT PRIMARY KEY, "
                         "date DATE, "
                         "check_in_time TIME, "
                         "check_out_time TIME, "
                         "status ENUM('online', 'offline'), "
                         "CONSTRAINT fk_doctor_user FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE "
                         ")")
                .execute();
            std::cout << "onlineDoctors table created successfully." << std::endl;
        }

        if (reservates_exists)
        {
            std::cout << "reaservations table is exists." << std::endl;
            ForeignKeys::migrateReservations(dbManager);
        }
        else
        {
            std::cout << "reaservations table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE reaservations ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "user_id INT NOT NULL, "
                         "doctor_id INT NOT NULL, "
                         "pet_id INT, "
                         "reservation_type VARCHAR(30) NOT NULL, "
                         "date DATE, "
                         "time_slot VARCHAR(20), "
                         "status ENUM('预约成功', '预约失败', '已取消', '已到院') NOT NULL, "
                         "user_hidden TINYINT NOT NULL DEFAULT 0 COMMENT '用户是否隐藏预约信息', "
                         "user_hidden_at DATETIME NULL COMMENT '用户隐藏时间', "
                         "hidden_by INT NULL COMMENT '执行隐藏的用户ID', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "CONSTRAINT fk_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
                         "CONSTRAINT fk_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE, "
                         "CONSTRAINT fk_pet_id FOREIGN KEY (pet_id) REFERENCES pets(id) ON DELETE CASCADE, "
                         "INDEX idx_user_hidden (user_id, user_hidden), "
                         "INDEX idx_doctorId_date_slot (doctor_id, date, time_slot), "
                         "INDEX idx_petId_date (pet_id, date) "
                         ")")
                .execute();
            std::cout << "reaservations table created successfully." << std::endl;
        }

        if (orders_exists)
        {
            std::cout << "orders table is exists." << std::endl;
            ForeignKeys::migrateOrders(dbManager);
            Columns::migrateOrders(dbManager);
        }
        else
        {
            std::cout << "orders table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE orders ("
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "owner_id INT NOT NULL, "
                         "pet_id INT NOT NULL, "
                         "doctor_id INT NOT NULL, "
                         "order_type VARCHAR(255),"
                         "order_data VARCHAR(255), "
                         "order_status ENUM('待付款', '已付款', '已取消', '已退款', '部分退款') NOT NULL DEFAULT '待付款', "
                         "order_totalprice DECIMAL(18, 2), "
                         "user_hidden TINYINT NOT NULL DEFAULT 0 COMMENT '用户是否隐藏订单信息', "
                         "user_hidden_at DATETIME NULL COMMENT '用户隐藏时间', "
                         "hidden_by INT NULL COMMENT '执行隐藏的用户ID', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "CONSTRAINT fk_orders_owner_id FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE, "
                         "CONSTRAINT fk_orders_pet_id FOREIGN KEY (pet_id) REFERENCES pets(id) ON DELETE CASCADE, "
                         "CONSTRAINT fk_orders_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE, "
                         "INDEX idx_orders_owner_hidden (owner_id, user_hidden), "
                         "INDEX idx_orders_doctor_time (doctor_id, created_at), "
                         "INDEX idx_petId_time (pet_id, created_at) "
                         ")")
                .execute();
            std::cout << "orders table created successfully" << std::endl;
        }

        if (orderMedicines_exists)
        {
            std::cout << "orderMedicines table is exists." << std::endl;
            ForeignKeys::migrateOrderMedicines(dbManager);
        }
        else
        {
            std::cout << "orderMedicines table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE orderMedicines ( "
                         "id INT PRIMARY KEY AUTO_INCREMENT, "
                         "order_id INT NOT NULL, "
                         "medicine_id INT NOT NULL, "
                         "medicine_name VARCHAR(255), "
                         "quantity INT NOT NULL, "
                         "price DECIMAL(18, 2), "
                         "total_price DECIMAL(18, 2), "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "INDEX idx_order_id (order_id), "
                         "INDEX idx_medicine_id (medicine_id), "
                         "INDEX idx_order_medicine (order_id, medicine_id), "
                         "CONSTRAINT fk_order_medicines_order_id FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE CASCADE "
                         ")")
                .execute();
            std::cout << "orderMedicines table created successfully" << std::endl;
        }

        if (workTimeRecords_exists)
        {
            std::cout << "workTimeRecords table already exists." << std::endl;
            ForeignKeys::migrateWorkTimeRecords(dbManager);
        }
        else
        {
            std::cout << "workTimeRecords table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE workTimeRecords( "
                         "id INT AUTO_INCREMENT PRIMARY KEY, "
                         "doctor_id INT NOT NULL, "
                         "date DATE, "
                         "check_in_time TIME, "
                         "check_out_time TIME, "
                         "status ENUM('pending', 'approved', 'rejected'), "
                         "notes TEXT, "
                         "is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否软删除', "
                         "deleted_at DATETIME NULL COMMENT '软删除时间', "
                         "deleted_by INT NULL COMMENT '执行删除的用户ID', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                         "CONSTRAINT fk_worktime_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE, "
                         "INDEX idx_user_deleted (doctor_id, is_deleted) "
                         ")")
                .execute();
            std::cout << "workTimeRecords table created successfully." << std::endl;
        }

        if (system_operations_exists)
        {
            std::cout << "system_operations table is exists." << std::endl;
            session->sql("ALTER TABLE system_operations "
                         "MODIFY COLUMN system_role ENUM('总裁', '副总裁', '财务总监', '财务经理', '人事经理', '部门经理', '超级管理员', '仓库管理员', '医生', '护士', '普通用户') NULL")
                .execute();
        }
        else
        {
            std::cout << "system_operations table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE system_operations( "
                         "id INT AUTO_INCREMENT PRIMARY KEY, "
                         "category ENUM('系统类') NOT NULL DEFAULT '系统类', "
                         "system_role ENUM('总裁', '副总裁', '财务总监', '财务经理', '人事经理', '部门经理', '超级管理员', '仓库管理员', '医生', '护士', '普通用户') NULL, "
                         "operator VARCHAR(255) NOT NULL DEFAULT '系统', "
                         "module VARCHAR(255) NOT NULL DEFAULT '', "
                         "action VARCHAR(100) NOT NULL, "
                         "result ENUM('成功', '警告', '失败') NOT NULL DEFAULT '成功', "
                         "summary TEXT, "
                         "details TEXT, "
                         "source VARCHAR(255) NOT NULL DEFAULT 'system_operations', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "INDEX idx_time (created_at), "
                         "INDEX idx_system_category (category), "
                         "INDEX idx_system_result (result) "
                         ")")
                .execute();
            std::cout << "system_operations table created successfully." << std::endl;
        }

        if (user_operations_exists)
        {
            std::cout << "user_operations table is exists." << std::endl;
            ForeignKeys::migrateUserOperations(dbManager);
            session->sql("ALTER TABLE user_operations "
                         "MODIFY COLUMN user_role ENUM('总裁', '副总裁', '财务总监', '财务经理', '人事经理', '部门经理', '超级管理员', '仓库管理员', '医生', '护士', '普通用户') NULL")
                .execute();
        }
        else
        {
            std::cout << "user_operations table does not exist. Creating..." << std::endl;
            session->sql("CREATE TABLE user_operations( "
                         "id INT AUTO_INCREMENT PRIMARY KEY, "
                         "user_id INT NOT NULL, "
                         "category ENUM('用户类') NOT NULL DEFAULT '用户类', "
                         "user_role ENUM('总裁', '副总裁', '财务总监', '财务经理', '人事经理', '部门经理', '超级管理员', '仓库管理员', '医生', '护士', '普通用户') NULL, "
                         "operator VARCHAR(255) NOT NULL DEFAULT '', "
                         "module VARCHAR(255) NOT NULL DEFAULT '', "
                         "action VARCHAR(100) NOT NULL, "
                         "result ENUM('成功', '警告', '失败') NOT NULL DEFAULT '成功', "
                         "summary TEXT, "
                         "details TEXT, "
                         "source VARCHAR(255) NOT NULL DEFAULT 'user_operations', "
                         "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                         "INDEX idx_user_time (user_id, created_at), "
                         "INDEX idx_user_category (category), "
                         "INDEX idx_user_result (result), "
                         "CONSTRAINT fk_user_operations_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE "
                         ")")
                .execute();
            std::cout << "user_operations table created successfully." << std::endl;
        }

        tables_created = true;
    }
}
