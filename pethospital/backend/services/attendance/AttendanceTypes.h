#pragma once

#include <ctime>
#include <optional>
#include <string>

namespace Attendance
{
enum class DayStatus
{
    Normal,
    Late,
    EarlyLeave,
    LateAndEarly,
    MissingOut,
    Absent
};

struct PunchEvent
{
    int deviceId = 0;
    int userId = 0;
    std::string attendanceNo;
    std::string eventId;
    std::string rawEventHash;
    std::time_t punchedAt = 0;
    std::string verifyMode = "unknown";
    std::string source = "device";
    int createdBy = 0;
    std::string correctionNote;
};

struct WorkWindow
{
    std::string checkInStart = "00:00:00";
    std::string checkInEnd = "00:00:00";
    std::string checkOutStart = "00:00:00";
    std::string checkOutEnd = "00:00:00";
};

std::string toString(DayStatus status);
DayStatus dayStatusFromString(const std::string &status);
}
