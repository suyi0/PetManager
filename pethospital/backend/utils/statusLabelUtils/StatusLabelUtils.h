#pragma once

#include <string>
#include <unordered_map>

namespace StatusLabelUtils
{
inline std::string mapValue(
    const std::string &value,
    const std::unordered_map<std::string, std::string> &mapping,
    const std::string &fallback = "")
{
    const auto it = mapping.find(value);
    if (it != mapping.end())
    {
        return it->second;
    }
    return fallback.empty() ? value : fallback;
}

inline std::string toDbReservationStatus(const std::string &status)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"预约成功", "scheduled"},
        {"预约失败", "failed"},
        {"已取消", "cancelled"},
        {"已到院", "arrived"},
        {"scheduled", "scheduled"},
        {"failed", "failed"},
        {"cancelled", "cancelled"},
        {"arrived", "arrived"},
    };
    return mapValue(status, mapping);
}

inline std::string toDisplayReservationStatus(const std::string &status)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"scheduled", "预约成功"},
        {"failed", "预约失败"},
        {"cancelled", "已取消"},
        {"arrived", "已到院"},
        {"预约成功", "预约成功"},
        {"预约失败", "预约失败"},
        {"已取消", "已取消"},
        {"已到院", "已到院"},
    };
    return mapValue(status, mapping, "预约成功");
}

inline bool isValidReservationStatus(const std::string &status)
{
    const std::string dbStatus = toDbReservationStatus(status);
    return dbStatus == "scheduled" || dbStatus == "failed" ||
           dbStatus == "cancelled" || dbStatus == "arrived";
}

inline std::string toDbOrderStatus(const std::string &status)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"待付款", "pending_payment"},
        {"已付款", "paid"},
        {"已取消", "cancelled"},
        {"已退款", "refunded"},
        {"部分退款", "partial_refund"},
        {"pending_payment", "pending_payment"},
        {"paid", "paid"},
        {"cancelled", "cancelled"},
        {"refunded", "refunded"},
        {"partial_refund", "partial_refund"},
    };
    return mapValue(status, mapping);
}

inline std::string toDisplayOrderStatus(const std::string &status)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"pending_payment", "待付款"},
        {"paid", "已付款"},
        {"cancelled", "已取消"},
        {"refunded", "已退款"},
        {"partial_refund", "部分退款"},
        {"待付款", "待付款"},
        {"已付款", "已付款"},
        {"已取消", "已取消"},
        {"已退款", "已退款"},
        {"部分退款", "部分退款"},
    };
    return mapValue(status, mapping, "待付款");
}

inline bool isValidOrderStatus(const std::string &status)
{
    const std::string dbStatus = toDbOrderStatus(status);
    return dbStatus == "pending_payment" || dbStatus == "paid" ||
           dbStatus == "cancelled" || dbStatus == "refunded" ||
           dbStatus == "partial_refund";
}

inline std::string toDbWorkTimeStatus(const std::string &status)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"正常", "normal"},
        {"迟到", "late"},
        {"早退", "early_leave"},
        {"加班", "overtime"},
        {"异常", "abnormal"},
        {"normal", "normal"},
        {"late", "late"},
        {"early_leave", "early_leave"},
        {"overtime", "overtime"},
        {"abnormal", "abnormal"},
    };
    return mapValue(status, mapping);
}

inline std::string toDisplayWorkTimeStatus(const std::string &status)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"normal", "正常"},
        {"late", "迟到"},
        {"early_leave", "早退"},
        {"overtime", "加班"},
        {"abnormal", "异常"},
        {"正常", "正常"},
        {"迟到", "迟到"},
        {"早退", "早退"},
        {"加班", "加班"},
        {"异常", "异常"},
    };
    return mapValue(status, mapping, "正常");
}

inline std::string toDbMedicalQueueStatus(const std::string &status)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"待接诊", "waiting"},
        {"接诊中", "in_progress"},
        {"已过号", "skipped"},
        {"已取消", "cancelled"},
        {"已完成", "completed"},
        {"waiting", "waiting"},
        {"in_progress", "in_progress"},
        {"skipped", "skipped"},
        {"cancelled", "cancelled"},
        {"completed", "completed"},
    };
    return mapValue(status, mapping);
}

inline std::string toDisplayMedicalQueueStatus(const std::string &status)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"waiting", "待接诊"},
        {"in_progress", "接诊中"},
        {"skipped", "已过号"},
        {"cancelled", "已取消"},
        {"completed", "已完成"},
        {"待接诊", "待接诊"},
        {"接诊中", "接诊中"},
        {"已过号", "已过号"},
        {"已取消", "已取消"},
        {"已完成", "已完成"},
    };
    return mapValue(status, mapping, "待接诊");
}

inline std::string toDbMedicalQueueSource(const std::string &source)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"预约", "appointment"},
        {"现场", "walk_in"},
        {"复诊", "follow_up"},
        {"急诊", "emergency"},
        {"appointment", "appointment"},
        {"walk_in", "walk_in"},
        {"follow_up", "follow_up"},
        {"emergency", "emergency"},
    };
    return mapValue(source, mapping);
}

inline std::string toDisplayMedicalQueueSource(const std::string &source)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"appointment", "预约"},
        {"walk_in", "现场"},
        {"follow_up", "复诊"},
        {"emergency", "急诊"},
        {"预约", "预约"},
        {"现场", "现场"},
        {"复诊", "复诊"},
        {"急诊", "急诊"},
    };
    return mapValue(source, mapping, "现场");
}

inline std::string toDbTriageLevel(const std::string &level)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"普通", "normal"},
        {"优先", "priority"},
        {"紧急", "urgent"},
        {"normal", "normal"},
        {"priority", "priority"},
        {"urgent", "urgent"},
    };
    return mapValue(level, mapping);
}

inline std::string toDisplayTriageLevel(const std::string &level)
{
    static const std::unordered_map<std::string, std::string> mapping = {
        {"normal", "普通"},
        {"priority", "优先"},
        {"urgent", "紧急"},
        {"普通", "普通"},
        {"优先", "优先"},
        {"紧急", "紧急"},
    };
    return mapValue(level, mapping, "普通");
}
}
