#pragma once

#include "AttendanceTypes.h"
#include "../../database/DatabaseManagerInterface.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace AttendanceService
{
Attendance::DayStatus evaluateStatus(
    const std::optional<std::time_t> &checkIn,
    const std::optional<std::time_t> &checkOut,
    const Attendance::WorkWindow &window);

std::string dateFromTime(std::time_t value);
std::string dateTimeFromTime(std::time_t value);
std::optional<std::time_t> parseDateTime(const std::string &value);

bool ingestPunch(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const Attendance::PunchEvent &event);

bool voidPunch(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    long long punchId,
    int operatorUserId,
    const std::string &reason);

void recalcDailyRecord(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId,
    const std::string &workDate,
    bool corrected = false,
    int correctedBy = 0,
    const std::string &correctionNote = "");

Attendance::WorkWindow loadWorkWindow(const std::shared_ptr<DatabaseManagerInterface> &dbManager);
// 指定日期优先取 attendance_workdays 当日窗口，缺省回退全局 workTimes（与日结/重算同一套窗口）。
Attendance::WorkWindow loadWorkWindow(const std::shared_ptr<DatabaseManagerInterface> &dbManager, const std::string &workDate);
int closeDay(const std::shared_ptr<DatabaseManagerInterface> &dbManager, const std::string &workDate);
nlohmann::json listMyRecords(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int userId, const std::string &month);
nlohmann::json listRecords(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int operatorUserId, const nlohmann::json &query);
}
