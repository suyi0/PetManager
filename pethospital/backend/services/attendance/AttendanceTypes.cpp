#include "AttendanceTypes.h"

namespace Attendance
{
std::string toString(DayStatus status)
{
    switch (status)
    {
    case DayStatus::Normal:
        return "normal";
    case DayStatus::Late:
        return "late";
    case DayStatus::EarlyLeave:
        return "early_leave";
    case DayStatus::LateAndEarly:
        return "late_and_early";
    case DayStatus::MissingOut:
        return "missing_out";
    case DayStatus::Absent:
        return "absent";
    }
    return "absent";
}

DayStatus dayStatusFromString(const std::string &status)
{
    if (status == "normal")
        return DayStatus::Normal;
    if (status == "late")
        return DayStatus::Late;
    if (status == "early_leave")
        return DayStatus::EarlyLeave;
    if (status == "late_and_early")
        return DayStatus::LateAndEarly;
    if (status == "missing_out")
        return DayStatus::MissingOut;
    return DayStatus::Absent;
}
}
