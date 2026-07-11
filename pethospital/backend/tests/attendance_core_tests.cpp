#include "../services/attendance/AttendanceSecurity.h"
#include "../services/attendance/AttendanceService.h"
#include "../services/attendance/AttendanceTypes.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

#ifndef ATTENDANCE_ROUTES_SOURCE_PATH
#error "ATTENDANCE_ROUTES_SOURCE_PATH is required"
#endif
#ifndef ATTENDANCE_SERVICE_SOURCE_PATH
#error "ATTENDANCE_SERVICE_SOURCE_PATH is required"
#endif
#ifndef DOCTOR_HANDLER_SOURCE_PATH
#error "DOCTOR_HANDLER_SOURCE_PATH is required"
#endif
#ifndef COLUMN_MIGRATIONS_SOURCE_PATH
#error "COLUMN_MIGRATIONS_SOURCE_PATH is required"
#endif

namespace
{
std::time_t at(const std::string &value)
{
    auto parsed = AttendanceService::parseDateTime(value);
    assert(parsed.has_value());
    return *parsed;
}

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

std::size_t countOccurrences(const std::string &source, const std::string &needle)
{
    std::size_t count = 0;
    for (std::size_t pos = source.find(needle); pos != std::string::npos; pos = source.find(needle, pos + needle.size()))
    {
        ++count;
    }
    return count;
}
}

int main()
{
    Attendance::WorkWindow window;
    window.checkInStart = "08:30:00";
    window.checkInEnd = "09:00:00";
    window.checkOutStart = "18:00:00";
    window.checkOutEnd = "18:30:00";

    assert(AttendanceService::evaluateStatus(at("2026-07-09 08:55:00"), at("2026-07-09 18:05:00"), window) == Attendance::DayStatus::Normal);
    assert(AttendanceService::evaluateStatus(at("2026-07-09 09:01:00"), at("2026-07-09 18:05:00"), window) == Attendance::DayStatus::Late);
    assert(AttendanceService::evaluateStatus(at("2026-07-09 08:55:00"), at("2026-07-09 17:59:00"), window) == Attendance::DayStatus::EarlyLeave);
    assert(AttendanceService::evaluateStatus(at("2026-07-09 09:01:00"), at("2026-07-09 17:59:00"), window) == Attendance::DayStatus::LateAndEarly);
    assert(AttendanceService::evaluateStatus(at("2026-07-09 08:55:00"), std::nullopt, window) == Attendance::DayStatus::MissingOut);
    assert(AttendanceService::evaluateStatus(std::nullopt, std::nullopt, window) == Attendance::DayStatus::Absent);
    assert(!AttendanceService::parseDateTime("2026-02-30 08:00:00").has_value());
    assert(!AttendanceService::parseDateTime("2026-07-09 08:00:00 trailing").has_value());
    assert(!AttendanceService::parseDateTime("2026-7-9 08:00:00").has_value());

    const std::string body = R"({"attendance_no":"10001","punched_at":"2026-07-09 08:55:30","verify_mode":"face","event_id":"dev-seq-000123"})";
    const std::string canonical = AttendanceSecurity::buildCanonicalString("POST", "/api/device/attendance/punch", "1783510000", "nonce-1", body);
    const std::string signature = AttendanceSecurity::hmacSha256Hex(canonical, "secret");
    // 已知向量：与 bin/attendance_device_sim.py 及 openssl 独立计算结果一致，锁定跨实现协议兼容。
    assert(signature == "4aef9b07597cacb44a86ee14d5187a8531699ce25cbb38c6ffeceeef5569035e");
    assert(AttendanceSecurity::verifySignature("POST", "/api/device/attendance/punch", "1783510000", "nonce-1", body, "secret", signature));
    assert(!AttendanceSecurity::verifySignature("POST", "/api/device/attendance/punch", "1783510000", "nonce-2", body, "secret", signature));
    assert(!AttendanceSecurity::verifySignature("POST", "/api/device/attendance/punch", "1783510001", "nonce-1", body, "secret", signature));
    assert(!AttendanceSecurity::verifySignature("POST", "/api/device/attendance/punch", "1783510000", "nonce-1", body, "wrong-secret", signature));
    assert(AttendanceSecurity::sha256Hex(body).size() == 64);

    // ===== 源码回归断言（与 rbac_route_tests 同一套路）：守住评审修掉的缺陷不复发 =====
    const std::string routes = readFile(ATTENDANCE_ROUTES_SOURCE_PATH);
    const std::string service = readFile(ATTENDANCE_SERVICE_SOURCE_PATH);
    const std::string doctorHandler = readFile(DOCTOR_HANDLER_SOURCE_PATH);
    const std::string columnMigrations = readFile(COLUMN_MIGRATIONS_SOURCE_PATH);

    // B1: attendance:manage 全部写操作有审计
    assert(countOccurrences(routes, "OperationLogger::logUserOperation") >= 6);
    // B4/A3: 设备打卡与手动补卡走同一个医生联动函数
    assert(countOccurrences(routes, "syncDoctorDutyFromDevicePunch(dbManager, event.userId, event.punchedAt)") >= 2);
    // A3: 手动签到/签退必须盖 last_attendance_event_at
    assert(countOccurrences(doctorHandler, "last_attendance_event_at = NOW()") >= 2);
    assertContains(doctorHandler, "last_attendance_event_at) ");
    // C4: nonce 重放幂等返回，且非重复键错误不吞
    assertContains(routes, "duplicateNonce");
    // C1: 设备事件日志带 device_key
    assertContains(routes, "device_key, event_type");
    // B6: 新设备回填人员下发队列
    assertContains(routes, "DevicePersonSync::enqueueDeviceBackfill");
    // A2: 手动 punch（device_id NULL）显式查重
    assertContains(service, "device_id IS NULL AND event_id = ?");
    // A2: 同值 min/max 不得伪造签退
    assertContains(service, "rowString(punchRow, 1) != rowString(punchRow, 0)");
    // B2: 管理端记录查询分页
    assertContains(service, "LIMIT ? OFFSET ?");
    // A1: 存量员工 attendance_no 回填
    assertContains(columnMigrations, "UPDATE users SET attendance_no = CAST(id AS CHAR)");
    // 设备协议 v1: 心跳对时、离线批量补推、人员同步领取与确认。
    assertContains(routes, "server_timestamp");
    assertContains(routes, "/api/device/attendance/punches");
    assertContains(routes, "/api/device/attendance/person-sync/pull");
    assertContains(routes, "/api/device/attendance/person-sync/ack");
    assertContains(routes, "kMaxDeviceBodyBytes");

    return 0;
}
