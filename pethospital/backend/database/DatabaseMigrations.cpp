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
    bool workTimes_exists = false;
    bool onlineDoctors_exists = false;
    bool reservates_exists = false;
    bool orders_exists = false;
    bool orderMedicines_exists = false;
    bool pets_exists = false;
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
        session->sql("INSERT INTO types (type) VALUES ('超级管理员'), ('医生'), ('仓库管理员'), ('普通用户')").execute();
        std::cout << "types table created successfully." << std::endl;
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
                     "type_id INT, "
                     "name VARCHAR(255), "
                     "password VARCHAR(255), "
                     "phone VARCHAR(20), "
                     "email VARCHAR(255), "
                     "birthday DATE, "
                     "address_id int, "
                     "head_image VARCHAR(255),"
                     "user_specialty VARCHAR(255),"
                     "user_introduction TEXT, "
                     "user_level int, "
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                     "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                     "CONSTRAINT fk_user_type FOREIGN KEY (type_id) REFERENCES types(id) ON DELETE CASCADE, "
                     "INDEX idx_users_name (name)"
                     ")")
            .execute();
        std::cout << "users table created successfully." << std::endl;
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
                     "user_id INT, "
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
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                     "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                     "INDEX idx_id_exp (id, days_until_expire), "
                     "INDEX idx_exp (days_until_expire) "
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
                     "CONSTRAINT fk_doctor_user FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE"
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
                     "user_id INT , "
                     "doctor_id INT, "
                     "date DATE, "
                     "time_slot VARCHAR(20), "
                     "status VARCHAR(20), "
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                     "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                     "CONSTRAINT fk_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
                     "CONSTRAINT fk_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE, "
                     "INDEX idx_userId_creationTime (user_id, created_at) "
                     ")")
            .execute();
        std::cout << "reaservations table created successfully." << std::endl;
    }

    if (pets_exists)
    {
        std::cout << "pets table is exists." << std::endl;
        ForeignKeys::migratePets(dbManager);
    }
    else
    {
        std::cout << "pets table does not exist. Creating..." << std::endl;
        session->sql("CREATE TABLE pets ( "
                     "id INT PRIMARY KEY AUTO_INCREMENT, "
                     "user_id INT, "
                     "pet_name VARCHAR(255), "
                     "pet_type VARCHAR(255), "
                     "pet_age VARCHAR(255), "
                     "pet_sex VARCHAR(255), "
                     "CONSTRAINT fk_pets_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE "
                     ")")
            .execute();
        std::cout << "pets table created successfully" << std::endl;
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
                     "pet_id INT, "
                     "doctor_id INT, "
                     "order_type VARCHAR(255),"
                     "order_data VARCHAR(255), "
                     "order_status VARCHAR(255), "
                     "order_totalprice DECIMAL(18, 2), "
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                     "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                     "CONSTRAINT fk_orders_pet_id FOREIGN KEY (pet_id) REFERENCES pets(id) ON DELETE CASCADE, "
                     "CONSTRAINT fk_orders_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE, "
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
                     "order_id INT, "
                     "medicine_id INT, "
                     "quantity INT, "
                     "price DECIMAL(18, 2), "
                     "total_price DECIMAL(18, 2), "
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                     "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                     "INDEX idx_orderId_time (order_id, created_at), "
                     "CONSTRAINT fk_order_id FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE CASCADE, "
                     "CONSTRAINT fk_medicine_id FOREIGN KEY (medicine_id) REFERENCES warehouse(id) ON DELETE CASCADE "
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
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                     "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, "
                     "CONSTRAINT fk_worktime_doctor_id FOREIGN KEY (doctor_id) REFERENCES users(id) ON DELETE CASCADE, "
                     "INDEX idx_user (doctor_id)"
                     ")")
            .execute();
        std::cout << "workTimeRecords table created successfully." << std::endl;
    }

    if (system_operations_exists)
    {
        std::cout << "system_operations table is exists." << std::endl;
    }
    else
    {
        std::cout << "system_operations table does not exist. Creating..." << std::endl;
        session->sql("CREATE TABLE system_operations( "
                     "id INT AUTO_INCREMENT PRIMARY KEY, "
                     "operation VARCHAR(100) NOT NULL, "
                     "details TEXT, "
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                     "INDEX idx_time (created_at)"
                     ")")
            .execute();
        std::cout << "system_operations table created successfully." << std::endl;
    }

    if (user_operations_exists)
    {
        std::cout << "user_operations table is exists." << std::endl;
        ForeignKeys::migrateUserOperations(dbManager);
    }
    else
    {
        std::cout << "user_operations table does not exist. Creating..." << std::endl;
        session->sql("CREATE TABLE user_operations( "
                     "id INT AUTO_INCREMENT PRIMARY KEY, "
                     "user_id INT NOT NULL, "
                     "operation VARCHAR(100) NOT NULL, "
                     "details TEXT, "
                     "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                     "INDEX idx_user_time (user_id, created_at), "
                     "CONSTRAINT fk_user_operations_user_id FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE "
                     ")")
            .execute();
        std::cout << "user_operations table created successfully." << std::endl;
    }

    tables_created = true;
}
}
