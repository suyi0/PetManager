#include "DevicePersonSync.h"

#include <iostream>

namespace
{
bool canUse(const std::shared_ptr<DatabaseManagerInterface> &dbManager)
{
    return dbManager && dbManager->getSession();
}
}

namespace DevicePersonSync
{
std::string ensureAttendanceNo(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int userId)
{
    if (!canUse(dbManager) || userId <= 0)
    {
        return "";
    }
    try
    {
        auto *session = dbManager->getSession();
        session->sql("UPDATE users SET attendance_no = CAST(id AS CHAR) "
                     "WHERE id = ? AND account_type = 'staff' AND is_deleted = 0 "
                     "AND (attendance_no IS NULL OR attendance_no = '')")
            .bind(userId)
            .execute();
        mysqlx::SqlResult result = session->sql("SELECT COALESCE(attendance_no, '') FROM users "
                                                "WHERE id = ? AND account_type = 'staff' AND is_deleted = 0 LIMIT 1")
                                       .bind(userId)
                                       .execute();
        mysqlx::Row row = result.fetchOne();
        return row ? row[0].get<std::string>() : "";
    }
    catch (const std::exception &e)
    {
        std::cerr << "DevicePersonSync ensureAttendanceNo failed: " << e.what() << std::endl;
        return "";
    }
}

void enqueueUpsert(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int userId)
{
    if (!canUse(dbManager) || userId <= 0)
    {
        return;
    }
    try
    {
        const std::string attendanceNo = ensureAttendanceNo(dbManager, userId);
        if (attendanceNo.empty())
        {
            return;
        }
        dbManager->getSession()
            ->sql("INSERT INTO device_person_sync (device_id, user_id, attendance_no, desired_state, sync_state) "
                  "SELECT d.id, ?, ?, 'active', 'pending' FROM attendance_devices AS d WHERE d.is_active = 1 "
                  "ON DUPLICATE KEY UPDATE attendance_no = VALUES(attendance_no), "
                  "desired_state = 'active', sync_state = 'pending', last_error = ''")
            .bind(userId)
            .bind(attendanceNo)
            .execute();
    }
    catch (const std::exception &e)
    {
        std::cerr << "DevicePersonSync enqueueUpsert failed: " << e.what() << std::endl;
    }
}

void enqueueRemove(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int userId)
{
    if (!canUse(dbManager) || userId <= 0)
    {
        return;
    }
    try
    {
        dbManager->getSession()
            ->sql("UPDATE device_person_sync SET desired_state = 'deleted', sync_state = 'pending', last_error = '' "
                  "WHERE user_id = ? AND (desired_state <> 'deleted' OR sync_state <> 'synced')")
            .bind(userId)
            .execute();
    }
    catch (const std::exception &e)
    {
        std::cerr << "DevicePersonSync enqueueRemove failed: " << e.what() << std::endl;
    }
}

void enqueueDeviceBackfill(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int deviceId)
{
    if (!canUse(dbManager) || deviceId <= 0)
    {
        return;
    }
    try
    {
        dbManager->getSession()
            ->sql("INSERT INTO device_person_sync (device_id, user_id, attendance_no, desired_state, sync_state) "
                  "SELECT ?, u.id, u.attendance_no, 'active', 'pending' FROM users AS u "
                  "WHERE u.account_type = 'staff' AND u.is_deleted = 0 AND COALESCE(u.attendance_no, '') <> '' "
                  "ON DUPLICATE KEY UPDATE attendance_no = VALUES(attendance_no), "
                  "desired_state = 'active', sync_state = 'pending', last_error = ''")
            .bind(deviceId)
            .execute();
    }
    catch (const std::exception &e)
    {
        std::cerr << "DevicePersonSync enqueueDeviceBackfill failed: " << e.what() << std::endl;
    }
}
}
