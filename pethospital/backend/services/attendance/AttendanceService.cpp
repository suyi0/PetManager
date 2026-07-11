#include "AttendanceService.h"

#include "../rbac/RbacService.h"

#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
int secondsOfDay(std::time_t value)
{
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &value);
#else
    localtime_r(&value, &tm);
#endif
    return tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec;
}

int parseTimeOfDay(const std::string &value)
{
    int hour = 0;
    int minute = 0;
    int second = 0;
    char c1 = '\0';
    char c2 = '\0';
    std::istringstream stream(value);
    stream >> hour >> c1 >> minute >> c2 >> second;
    if (!stream || c1 != ':' || c2 != ':' || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
    {
        return 0;
    }
    return hour * 3600 + minute * 60 + second;
}

std::string rowString(const mysqlx::Row &row, int index)
{
    return row[index].isNull() ? "" : row[index].get<std::string>();
}

int rowInt(const mysqlx::Row &row, int index)
{
    return row[index].isNull() ? 0 : row[index].get<int>();
}

Attendance::WorkWindow loadWorkWindowForDate(const std::shared_ptr<DatabaseManagerInterface> &dbManager, const std::string &workDate)
{
    Attendance::WorkWindow window;
    if (!dbManager || !dbManager->getSession())
    {
        return window;
    }

    if (!workDate.empty())
    {
        mysqlx::SqlResult dayResult = dbManager->getSession()
                                           ->sql("SELECT CAST(check_in_start AS CHAR), CAST(check_in_end AS CHAR), "
                                                 "CAST(check_out_start AS CHAR), CAST(check_out_end AS CHAR) "
                                                 "FROM attendance_workdays WHERE work_date = ? AND day_type = 'workday' LIMIT 1")
                                           .bind(workDate)
                                           .execute();
        mysqlx::Row dayRow = dayResult.fetchOne();
        if (dayRow)
        {
            window.checkInStart = rowString(dayRow, 0);
            window.checkInEnd = rowString(dayRow, 1);
            window.checkOutStart = rowString(dayRow, 2);
            window.checkOutEnd = rowString(dayRow, 3);
            return window;
        }
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT CAST(check_in_time_start AS CHAR), CAST(check_in_time_end AS CHAR), "
                                        "CAST(check_out_time_start AS CHAR), CAST(check_out_time_end AS CHAR) FROM workTimes LIMIT 1")
                                  .execute();
    mysqlx::Row row = result.fetchOne();
    if (!row)
    {
        return window;
    }
    window.checkInStart = rowString(row, 0);
    window.checkInEnd = rowString(row, 1);
    window.checkOutStart = rowString(row, 2);
    window.checkOutEnd = rowString(row, 3);
    return window;
}
}

namespace AttendanceService
{
Attendance::DayStatus evaluateStatus(
    const std::optional<std::time_t> &checkIn,
    const std::optional<std::time_t> &checkOut,
    const Attendance::WorkWindow &window)
{
    if (!checkIn)
    {
        return Attendance::DayStatus::Absent;
    }
    if (!checkOut)
    {
        return Attendance::DayStatus::MissingOut;
    }

    const bool late = secondsOfDay(*checkIn) > parseTimeOfDay(window.checkInEnd);
    const bool early = secondsOfDay(*checkOut) < parseTimeOfDay(window.checkOutStart);
    if (late && early)
    {
        return Attendance::DayStatus::LateAndEarly;
    }
    if (late)
    {
        return Attendance::DayStatus::Late;
    }
    if (early)
    {
        return Attendance::DayStatus::EarlyLeave;
    }
    return Attendance::DayStatus::Normal;
}

std::string dateFromTime(std::time_t value)
{
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &value);
#else
    localtime_r(&value, &tm);
#endif
    std::ostringstream stream;
    stream << std::put_time(&tm, "%Y-%m-%d");
    return stream.str();
}

std::string dateTimeFromTime(std::time_t value)
{
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &value);
#else
    localtime_r(&value, &tm);
#endif
    std::ostringstream stream;
    stream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

std::optional<std::time_t> parseDateTime(const std::string &value)
{
    if (value.size() != 19)
    {
        return std::nullopt;
    }
    std::tm tm{};
    std::istringstream stream(value);
    stream >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (!stream || !stream.eof())
    {
        return std::nullopt;
    }
    tm.tm_isdst = -1;
    const std::time_t parsed = std::mktime(&tm);
    if (parsed == static_cast<std::time_t>(-1) || dateTimeFromTime(parsed) != value)
    {
        return std::nullopt;
    }
    return parsed;
}

Attendance::WorkWindow loadWorkWindow(const std::shared_ptr<DatabaseManagerInterface> &dbManager)
{
    return loadWorkWindowForDate(dbManager, "");
}

Attendance::WorkWindow loadWorkWindow(const std::shared_ptr<DatabaseManagerInterface> &dbManager, const std::string &workDate)
{
    return loadWorkWindowForDate(dbManager, workDate);
}

int closeDay(const std::shared_ptr<DatabaseManagerInterface> &dbManager, const std::string &workDate)
{
    if (!dbManager || !dbManager->getSession() || workDate.empty())
    {
        return 0;
    }

    auto *session = dbManager->getSession();
    mysqlx::SqlResult workdayResult = session->sql(
                                             "SELECT id FROM attendance_workdays WHERE work_date = ? AND day_type = 'workday' LIMIT 1")
                                         .bind(workDate)
                                         .execute();
    if (!workdayResult.fetchOne())
    {
        return 0;
    }

    mysqlx::SqlResult staffResult = session->sql(
                                            "SELECT u.id, COALESCE(pos.id, 0), COALESCE(pos.department_id, 0), COALESCE(d.branch_id, 0) "
                                            "FROM users AS u "
                                            "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                            "LEFT JOIN departments AS d ON d.id = pos.department_id "
                                            "LEFT JOIN attendance_records AS ar ON ar.user_id = u.id AND ar.work_date = ? "
                                            "WHERE u.account_type = 'staff' AND u.is_deleted = 0 "
                                            "AND COALESCE(u.attendance_no, '') <> '' AND ar.id IS NULL "
                                            "AND u.created_at < DATE_ADD(?, INTERVAL 1 DAY)")
                                        .bind(workDate)
                                        .bind(workDate)
                                        .execute();

    int inserted = 0;
    for (mysqlx::Row row = staffResult.fetchOne(); row; row = staffResult.fetchOne())
    {
        session->sql("INSERT IGNORE INTO attendance_records "
                     "(user_id, work_date, status, branch_id, department_id, position_id) VALUES (?, ?, 'absent', ?, ?, ?)")
            .bind(rowInt(row, 0))
            .bind(workDate)
            .bind(rowInt(row, 3) > 0 ? mysqlx::Value(rowInt(row, 3)) : mysqlx::Value())
            .bind(rowInt(row, 2) > 0 ? mysqlx::Value(rowInt(row, 2)) : mysqlx::Value())
            .bind(rowInt(row, 1) > 0 ? mysqlx::Value(rowInt(row, 1)) : mysqlx::Value())
            .execute();
        ++inserted;
    }
    return inserted;
}

bool ingestPunch(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const Attendance::PunchEvent &event)
{
    if (!dbManager || !dbManager->getSession() || event.userId <= 0 || event.punchedAt <= 0)
    {
        return false;
    }

    try
    {
        auto *session = dbManager->getSession();
        // 手动/导入 punch 的 device_id 为 NULL，(device_id, event_id) 唯一键对 NULL 行
        // 不去重（MySQL NULL 语义），必须显式查重，否则补卡双击会产生重复事实。
        if (event.deviceId <= 0 && !event.eventId.empty())
        {
            mysqlx::SqlResult dup = session->sql("SELECT id FROM attendance_punches "
                                                 "WHERE device_id IS NULL AND event_id = ? LIMIT 1")
                                        .bind(event.eventId)
                                        .execute();
            if (dup.fetchOne())
            {
                recalcDailyRecord(dbManager, event.userId, dateFromTime(event.punchedAt), event.source == "manual", event.createdBy, event.correctionNote);
                return true;
            }
        }
        try
        {
            session->sql("INSERT INTO attendance_punches "
                         "(device_id, user_id, attendance_no, event_id, raw_event_hash, punched_at, verify_mode, source, created_by) "
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)")
                .bind(event.deviceId > 0 ? mysqlx::Value(event.deviceId) : mysqlx::Value())
                .bind(event.userId)
                .bind(event.attendanceNo)
                .bind(event.eventId.empty() ? mysqlx::Value() : mysqlx::Value(event.eventId))
                .bind(event.rawEventHash)
                .bind(dateTimeFromTime(event.punchedAt))
                .bind(event.verifyMode)
                .bind(event.source)
                .bind(event.createdBy > 0 ? mysqlx::Value(event.createdBy) : mysqlx::Value())
                .execute();
        }
        catch (const mysqlx::Error &e)
        {
            const std::string message = e.what();
            if (message.find("Duplicate") == std::string::npos && message.find("duplicate") == std::string::npos)
            {
                throw;
            }
        }
        recalcDailyRecord(
            dbManager,
            event.userId,
            dateFromTime(event.punchedAt),
            event.source == "manual",
            event.createdBy,
            event.correctionNote);
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Attendance ingest failed: " << e.what() << std::endl;
        return false;
    }
}

bool voidPunch(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    long long punchId,
    int operatorUserId,
    const std::string &reason)
{
    if (!dbManager || !dbManager->getSession() || punchId <= 0 || operatorUserId <= 0)
    {
        return false;
    }

    auto *session = dbManager->getSession();
    mysqlx::SqlResult lookup = session->sql(
                                          "SELECT user_id, CAST(punched_at AS CHAR) FROM attendance_punches WHERE id = ? AND is_voided = 0 LIMIT 1")
                                      .bind(punchId)
                                      .execute();
    mysqlx::Row row = lookup.fetchOne();
    if (!row)
    {
        return false;
    }

    const int userId = rowInt(row, 0);
    const auto punchedAt = parseDateTime(rowString(row, 1));
    if (!punchedAt)
    {
        return false;
    }

    mysqlx::SqlResult update = session->sql(
                                          "UPDATE attendance_punches SET is_voided = 1, voided_by = ?, voided_at = NOW(), void_reason = ? "
                                          "WHERE id = ? AND is_voided = 0")
                                      .bind(operatorUserId)
                                      .bind(reason)
                                      .bind(punchId)
                                      .execute();
    if (update.getAffectedItemsCount() == 0)
    {
        return false;
    }

    recalcDailyRecord(dbManager, userId, dateFromTime(*punchedAt), true, operatorUserId, reason);
    return true;
}

void recalcDailyRecord(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId,
    const std::string &workDate,
    bool corrected,
    int correctedBy,
    const std::string &correctionNote)
{
    if (!dbManager || !dbManager->getSession() || userId <= 0 || workDate.empty())
    {
        return;
    }

    auto *session = dbManager->getSession();
    mysqlx::SqlResult punchResult = session->sql(
                                            "SELECT CAST(MIN(punched_at) AS CHAR), CAST(MAX(punched_at) AS CHAR), COUNT(*) "
                                            "FROM attendance_punches "
                                            "WHERE user_id = ? AND DATE(punched_at) = ? AND is_voided = 0")
                                        .bind(userId)
                                        .bind(workDate)
                                        .execute();
    mysqlx::Row punchRow = punchResult.fetchOne();
    std::optional<std::time_t> checkIn;
    std::optional<std::time_t> checkOut;
    int punchCount = 0;
    if (punchRow)
    {
        punchCount = rowInt(punchRow, 2);
        if (punchCount > 0)
        {
            checkIn = parseDateTime(rowString(punchRow, 0));
            // 设计 §6.3：最晚 punch 等于最早 punch 时视为未签退。按值而非按行数判断，
            // 否则同一秒的重复事实（多设备/重复补卡）会伪造出"零时长签退"。
            if (punchCount > 1 && rowString(punchRow, 1) != rowString(punchRow, 0))
            {
                checkOut = parseDateTime(rowString(punchRow, 1));
            }
        }
    }

    const Attendance::DayStatus status = evaluateStatus(checkIn, checkOut, loadWorkWindowForDate(dbManager, workDate));

    mysqlx::SqlResult orgResult = session->sql(
                                          "SELECT COALESCE(pos.id, 0), COALESCE(pos.department_id, 0), COALESCE(d.branch_id, 0) "
                                          "FROM users AS u "
                                          "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                          "LEFT JOIN departments AS d ON d.id = pos.department_id "
                                          "WHERE u.id = ? LIMIT 1")
                                      .bind(userId)
                                      .execute();
    mysqlx::Row orgRow = orgResult.fetchOne();
    const int positionId = orgRow ? rowInt(orgRow, 0) : 0;
    const int departmentId = orgRow ? rowInt(orgRow, 1) : 0;
    const int branchId = orgRow ? rowInt(orgRow, 2) : 0;

    session->sql("INSERT INTO attendance_records "
                 "(user_id, work_date, check_in_at, check_out_at, status, is_corrected, corrected_by, corrected_at, correction_note, branch_id, department_id, position_id) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?, CASE WHEN ? = 1 THEN NOW() ELSE NULL END, ?, ?, ?, ?) "
                 "ON DUPLICATE KEY UPDATE "
                 "check_in_at = VALUES(check_in_at), check_out_at = VALUES(check_out_at), status = VALUES(status), "
                 "is_corrected = GREATEST(is_corrected, VALUES(is_corrected)), "
                 "corrected_by = CASE WHEN VALUES(is_corrected) = 1 THEN VALUES(corrected_by) ELSE corrected_by END, "
                 "corrected_at = CASE WHEN VALUES(is_corrected) = 1 THEN NOW() ELSE corrected_at END, "
                 "correction_note = CASE WHEN VALUES(is_corrected) = 1 THEN VALUES(correction_note) ELSE correction_note END")
        .bind(userId)
        .bind(workDate)
        .bind(checkIn ? mysqlx::Value(dateTimeFromTime(*checkIn)) : mysqlx::Value())
        .bind(checkOut ? mysqlx::Value(dateTimeFromTime(*checkOut)) : mysqlx::Value())
        .bind(Attendance::toString(status))
        .bind(corrected ? 1 : 0)
        .bind(correctedBy > 0 ? mysqlx::Value(correctedBy) : mysqlx::Value())
        .bind(corrected ? 1 : 0)
        .bind(correctionNote)
        .bind(branchId > 0 ? mysqlx::Value(branchId) : mysqlx::Value())
        .bind(departmentId > 0 ? mysqlx::Value(departmentId) : mysqlx::Value())
        .bind(positionId > 0 ? mysqlx::Value(positionId) : mysqlx::Value())
        .execute();
}

nlohmann::json listMyRecords(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int userId, const std::string &month)
{
    nlohmann::json items = nlohmann::json::array();
    if (!dbManager || !dbManager->getSession() || userId <= 0)
    {
        return items;
    }
    std::string start = month.empty() ? dateFromTime(std::time(nullptr)).substr(0, 7) : month;
    mysqlx::SqlResult result = dbManager->getSession()
                                  ->sql("SELECT CAST(work_date AS CHAR), CAST(check_in_at AS CHAR), CAST(check_out_at AS CHAR), status, is_corrected, correction_note "
                                        "FROM attendance_records WHERE user_id = ? AND DATE_FORMAT(work_date, '%Y-%m') = ? ORDER BY work_date ASC")
                                  .bind(userId)
                                  .bind(start)
                                  .execute();
    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
    {
        items.push_back({{"work_date", rowString(row, 0)},
                         {"check_in_at", rowString(row, 1)},
                         {"check_out_at", rowString(row, 2)},
                         {"status", rowString(row, 3)},
                         {"is_corrected", rowInt(row, 4) == 1},
                         {"correction_note", rowString(row, 5)}});
    }
    return items;
}

nlohmann::json listRecords(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int operatorUserId, const nlohmann::json &query)
{
    nlohmann::json items = nlohmann::json::array();
    if (!dbManager || !dbManager->getSession() || operatorUserId <= 0)
    {
        return {{"items", items}, {"total", 0}};
    }

    RbacService::EffectiveOrgScope scope = RbacService::loadEffectiveOrgScope(dbManager, operatorUserId);
    std::string where = " WHERE 1=1 ";
    std::vector<mysqlx::Value> binds;
    if (query.contains("month") && query["month"].is_string() && !query["month"].get<std::string>().empty())
    {
        where += " AND DATE_FORMAT(ar.work_date, '%Y-%m') = ? ";
        binds.emplace_back(query["month"].get<std::string>());
    }
    if (query.contains("start_date") && query["start_date"].is_string() && !query["start_date"].get<std::string>().empty())
    {
        where += " AND ar.work_date >= ? ";
        binds.emplace_back(query["start_date"].get<std::string>());
    }
    if (query.contains("end_date") && query["end_date"].is_string() && !query["end_date"].get<std::string>().empty())
    {
        where += " AND ar.work_date <= ? ";
        binds.emplace_back(query["end_date"].get<std::string>());
    }
    if (query.contains("user_id") && query["user_id"].is_number_integer() && query["user_id"].get<int>() > 0)
    {
        where += " AND ar.user_id = ? ";
        binds.emplace_back(query["user_id"].get<int>());
    }
    if (query.contains("status") && query["status"].is_string() && !query["status"].get<std::string>().empty())
    {
        where += " AND ar.status = ? ";
        binds.emplace_back(query["status"].get<std::string>());
    }

    std::vector<int> scopeDepartments = scope.departmentIds;
    if (!scope.unrestricted)
    {
        if (scopeDepartments.empty())
        {
            where += " AND ar.department_id = -1 ";
        }
        else
        {
            where += " AND ar.department_id IN (";
            for (std::size_t i = 0; i < scopeDepartments.size(); ++i)
            {
                where += (i == 0 ? "?" : ",?");
            }
            where += ") ";
        }
    }

    int page = 1;
    if (query.contains("page") && query["page"].is_number_integer() && query["page"].get<int>() > 0)
    {
        page = query["page"].get<int>();
    }
    int pageSize = 20;
    if (query.contains("page_size") && query["page_size"].is_number_integer())
    {
        pageSize = std::min(100, std::max(1, query["page_size"].get<int>()));
    }
    const long long offset = static_cast<long long>(page - 1) * pageSize;

    const std::string sql = "SELECT ar.user_id, u.name, CAST(ar.work_date AS CHAR), CAST(ar.check_in_at AS CHAR), "
                            "CAST(ar.check_out_at AS CHAR), ar.status, ar.is_corrected, ar.correction_note, "
                            "COALESCE(d.name, ''), COUNT(*) OVER() "
                            "FROM attendance_records AS ar "
                            "JOIN users AS u ON u.id = ar.user_id "
                            "LEFT JOIN departments AS d ON d.id = ar.department_id " +
                            where + " ORDER BY ar.work_date DESC, ar.user_id ASC LIMIT ? OFFSET ?";
    auto querySql = dbManager->getSession()->sql(sql);
    for (const mysqlx::Value &value : binds)
    {
        querySql.bind(value);
    }
    for (int departmentId : scopeDepartments)
    {
        querySql.bind(departmentId);
    }
    querySql.bind(pageSize);
    querySql.bind(offset);
    mysqlx::SqlResult result = querySql.execute();
    int total = 0;
    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
    {
        total = rowInt(row, 9);
        items.push_back({{"user_id", rowInt(row, 0)},
                         {"name", rowString(row, 1)},
                         {"work_date", rowString(row, 2)},
                         {"check_in_at", rowString(row, 3)},
                         {"check_out_at", rowString(row, 4)},
                         {"status", rowString(row, 5)},
                         {"is_corrected", rowInt(row, 6) == 1},
                         {"correction_note", rowString(row, 7)},
                         {"department_name", rowString(row, 8)}});
    }
    return {{"items", items}, {"total", total}, {"page", page}, {"page_size", pageSize}};
}
}
