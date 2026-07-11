#include "attendanceRoutes.h"

#include "../../services/attendance/AttendanceSecurity.h"
#include "../../services/attendance/AttendanceService.h"
#include "../../services/attendance/DevicePersonSync.h"
#include "../../services/logger/operationLogger.h"
#include "../../services/rbac/RbacService.h"
#include "../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../services/realtime/doctorListBroadcaster/doctorListBroadcaster.h"
#include "../../services/redis/doctorListCache/DoctorListCache.h"
#include "../../services/redis/RedisClient.h"
#include "../../utils/permissions/Permissions.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <openssl/rand.h>
#include <sstream>
#include <string>
#include <vector>

namespace
{
struct DeviceAuth
{
    int deviceId = 0;
    std::string secret;
    std::string deviceKey;
    std::string requestHash;        // body 的 SHA256，验签时已算过，链路内复用
    bool duplicateNonce = false;    // nonce 重放（设备离线重试）：幂等成功，不再处理 body
};

constexpr std::size_t kMaxDeviceBodyBytes = 64 * 1024;
constexpr int kMaxPunchBatchSize = 100;
constexpr int kMaxSyncBatchSize = 100;
constexpr const char *kDeviceProtocolVersion = "1.0";

struct DevicePunchResult
{
    bool accepted = false;
    std::string code;
    std::string message;
    int userId = 0;
    std::time_t punchedAt = 0;
};

// Crow 的 (req, res) 形态路由必须显式 res.end()，否则连接一直挂到客户端超时。
// 项目其他路由靠 OperationLogger::Finish* 终结；考勤路由用作用域守卫兜底全部
// return / 异常路径（已完成的响应不重复 end）。
struct ResponseEnder
{
    crow::response &res;
    ~ResponseEnder()
    {
        if (!res.is_completed())
        {
            res.end();
        }
    }
};

std::string jsonString(const nlohmann::json &body, const std::string &key, const std::string &fallback = "")
{
    return body.contains(key) && body[key].is_string() ? body[key].get<std::string>() : fallback;
}

int jsonInt(const nlohmann::json &body, const std::string &key, int fallback = 0)
{
    return body.contains(key) && body[key].is_number_integer() ? body[key].get<int>() : fallback;
}

long long jsonLongLong(const nlohmann::json &body, const std::string &key, long long fallback = 0)
{
    return body.contains(key) && body[key].is_number_integer() ? body[key].get<long long>() : fallback;
}

std::string randomHex(std::size_t bytes)
{
    std::vector<unsigned char> buffer(bytes);
    if (RAND_bytes(buffer.data(), static_cast<int>(buffer.size())) != 1)
    {
        throw std::runtime_error("RAND_bytes failed");
    }
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (unsigned char byte : buffer)
    {
        stream << std::setw(2) << static_cast<int>(byte);
    }
    return stream.str();
}

std::string queryParam(const crow::request &req, const char *key)
{
    const char *value = req.url_params.get(key);
    return value == nullptr ? "" : std::string(value);
}

bool isValidVerifyMode(const std::string &value)
{
    return value == "face" || value == "fingerprint" || value == "card" ||
           value == "password" || value == "manual" || value == "unknown";
}

bool isLowerHex(const std::string &value, std::size_t expectedLength)
{
    if (value.size() != expectedLength)
    {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char value)
                       { return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f'); });
}

std::optional<long long> parsePositiveInteger(const std::string &value)
{
    if (value.empty() || !std::all_of(value.begin(), value.end(), [](unsigned char character)
                                      { return character >= '0' && character <= '9'; }))
    {
        return std::nullopt;
    }
    try
    {
        std::size_t consumed = 0;
        const long long result = std::stoll(value, &consumed);
        return consumed == value.size() && result > 0 ? std::optional<long long>(result) : std::nullopt;
    }
    catch (const std::exception &)
    {
        return std::nullopt;
    }
}

nlohmann::json deviceClockPayload()
{
    const std::time_t now = std::time(nullptr);
    return {
        {"server_time", AttendanceService::dateTimeFromTime(now)},
        {"server_timestamp", static_cast<long long>(now)},
        {"protocol_version", kDeviceProtocolVersion},
    };
}

int secondsFromClock(const std::string &value)
{
    int hour = 0;
    int minute = 0;
    int second = 0;
    char c1 = '\0';
    char c2 = '\0';
    std::istringstream stream(value);
    stream >> hour >> c1 >> minute >> c2 >> second;
    if (!stream || c1 != ':' || c2 != ':')
    {
        return 0;
    }
    return hour * 3600 + minute * 60 + second;
}

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

bool isDateToday(std::time_t value)
{
    return AttendanceService::dateFromTime(value) == AttendanceService::dateFromTime(std::time(nullptr));
}

bool targetWithinOrgScope(const std::shared_ptr<DatabaseManagerInterface> &dbManager, int operatorUserId, int targetUserId)
{
    RbacService::EffectiveOrgScope scope = RbacService::loadEffectiveOrgScope(dbManager, operatorUserId);
    if (scope.unrestricted)
    {
        return true;
    }
    if (!dbManager || !dbManager->getSession())
    {
        return false;
    }
    mysqlx::SqlResult result = dbManager->getSession()
                                    ->sql("SELECT COALESCE(pos.department_id, 0) "
                                          "FROM users AS u LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                          "WHERE u.id = ? LIMIT 1")
                                    .bind(targetUserId)
                                    .execute();
    mysqlx::Row row = result.fetchOne();
    if (!row || row[0].isNull())
    {
        return false;
    }
    const int departmentId = row[0].get<int>();
    return std::find(scope.departmentIds.begin(), scope.departmentIds.end(), departmentId) != scope.departmentIds.end();
}

void syncDoctorDutyFromDevicePunch(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId,
    std::time_t punchedAt)
try
{
    if (!dbManager || !dbManager->getSession() || userId <= 0 || !isDateToday(punchedAt))
    {
        return;
    }

    mysqlx::SqlResult doctorResult = dbManager->getSession()
                                        ->sql("SELECT pos.staff_kind FROM users AS u "
                                              "JOIN positions AS pos ON pos.id = u.position_id "
                                              "WHERE u.id = ? AND pos.staff_kind = 'doctor' LIMIT 1")
                                        .bind(userId)
                                        .execute();
    if (!doctorResult.fetchOne())
    {
        return;
    }

    const std::string workDate = AttendanceService::dateFromTime(punchedAt);
    // 与考勤重算同一套窗口：优先当日 attendance_workdays，回退全局 workTimes。
    const Attendance::WorkWindow window = AttendanceService::loadWorkWindow(dbManager, workDate);
    const int punchSeconds = secondsOfDay(punchedAt);
    const int checkInStart = secondsFromClock(window.checkInStart);
    const int checkInEnd = secondsFromClock(window.checkInEnd);
    const int checkOutStart = secondsFromClock(window.checkOutStart);
    const int checkOutEnd = secondsFromClock(window.checkOutEnd);
    const std::string eventAt = AttendanceService::dateTimeFromTime(punchedAt);
    const std::string eventTime = eventAt.substr(11);

    bool changed = false;
    if (punchSeconds >= checkInStart && punchSeconds <= checkInEnd)
    {
        // check_out_time 列约定 NOT NULL，'00:00:00' 表示"未签退"（与遗留建表默认值一致），不能写 NULL。
        mysqlx::SqlResult result = dbManager->getSession()
                                      ->sql("INSERT INTO onlineDoctors (doctor_id, date, check_in_time, check_out_time, status, last_attendance_event_at) "
                                            "VALUES (?, ?, ?, '00:00:00', 'online', ?) "
                                            "ON DUPLICATE KEY UPDATE "
                                            "date = CASE WHEN last_attendance_event_at IS NULL OR last_attendance_event_at <= VALUES(last_attendance_event_at) THEN VALUES(date) ELSE date END, "
                                            "check_in_time = CASE WHEN last_attendance_event_at IS NULL OR last_attendance_event_at <= VALUES(last_attendance_event_at) THEN VALUES(check_in_time) ELSE check_in_time END, "
                                            "check_out_time = CASE WHEN last_attendance_event_at IS NULL OR last_attendance_event_at <= VALUES(last_attendance_event_at) THEN '00:00:00' ELSE check_out_time END, "
                                            "status = CASE WHEN last_attendance_event_at IS NULL OR last_attendance_event_at <= VALUES(last_attendance_event_at) THEN 'online' ELSE status END, "
                                            "last_attendance_event_at = GREATEST(COALESCE(last_attendance_event_at, VALUES(last_attendance_event_at)), VALUES(last_attendance_event_at))")
                                      .bind(userId)
                                      .bind(workDate)
                                      .bind(eventTime)
                                      .bind(eventAt)
                                      .execute();
        changed = result.getAffectedItemsCount() > 0;
    }
    else if (punchSeconds >= checkOutStart && punchSeconds <= checkOutEnd)
    {
        mysqlx::SqlResult result = dbManager->getSession()
                                      ->sql("UPDATE onlineDoctors SET check_out_time = ?, status = 'offline', last_attendance_event_at = ? "
                                            "WHERE doctor_id = ? AND date = ? "
                                            "AND (last_attendance_event_at IS NULL OR last_attendance_event_at <= ?)")
                                      .bind(eventTime)
                                      .bind(eventAt)
                                      .bind(userId)
                                      .bind(workDate)
                                      .bind(eventAt)
                                      .execute();
        changed = result.getAffectedItemsCount() > 0;
    }

    if (changed)
    {
        AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
        DoctorListCache::invalidateDoctorList();
        DoctorListBroadcaster::instance().notifyDoctorListChanged();
    }
}
catch (const std::exception &e)
{
    // 联动是配角：punch 已入库、记录已重算，联动失败只记日志，不能让设备侧收到 500 去重试打卡。
    std::cerr << "Doctor duty sync from punch failed: " << e.what() << std::endl;
}

void logDeviceEvent(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int deviceId,
    const std::string &deviceKey,
    const std::string &eventType,
    const std::string &result,
    const std::string &requestHash,
    const std::string &message)
{
    if (!dbManager || !dbManager->getSession())
    {
        return;
    }
    try
    {
        // device_key 单独落列：未知/未注册设备 device_id 为 NULL，排查"哪台设备在乱发"全靠它。
        dbManager->getSession()
            ->sql("INSERT INTO attendance_device_events (device_id, device_key, event_type, result, request_hash, message) VALUES (?, ?, ?, ?, ?, ?)")
            .bind(deviceId > 0 ? mysqlx::Value(deviceId) : mysqlx::Value())
            .bind(deviceKey.substr(0, 64))
            .bind(eventType)
            .bind(result)
            .bind(requestHash)
            .bind(message.substr(0, 255))
            .execute();
    }
    catch (const std::exception &)
    {
        // 设备事件日志不能影响主链路。
    }
}

std::optional<DeviceAuth> authenticateDevice(
    const crow::request &req,
    crow::response &res,
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const std::string &canonicalPath,
    const std::string &eventType)
{
    const std::string requestHash = AttendanceSecurity::sha256Hex(req.body);
    if (!dbManager || !dbManager->getSession())
    {
        res = ResponseHelper::system_error(req, "Database connection unavailable");
        return std::nullopt;
    }
    if (req.body.size() > kMaxDeviceBodyBytes)
    {
        res = ResponseHelper::validation(req, "设备请求体不能超过 64 KiB");
        return std::nullopt;
    }

    const std::string deviceKey = req.get_header_value("X-Device-Key");
    const std::string timestamp = req.get_header_value("X-Device-Timestamp");
    const std::string nonce = req.get_header_value("X-Device-Nonce");
    const std::string signature = req.get_header_value("X-Device-Sign");
    if (deviceKey.empty() || timestamp.empty() || nonce.empty() || signature.empty())
    {
        logDeviceEvent(dbManager, 0, deviceKey, eventType, "failure", requestHash, "missing device auth headers");
        res = ResponseHelper::unauthorized(req, "设备认证头缺失");
        return std::nullopt;
    }
    if (deviceKey.size() > 64 || nonce.size() > 128 || !isLowerHex(signature, 64))
    {
        logDeviceEvent(dbManager, 0, deviceKey, eventType, "failure", requestHash, "invalid device auth header format");
        res = ResponseHelper::unauthorized(req, "设备认证头格式无效");
        return std::nullopt;
    }

    const std::optional<long long> clientTs = parsePositiveInteger(timestamp);
    if (!clientTs)
    {
        logDeviceEvent(dbManager, 0, deviceKey, eventType, "failure", requestHash, "invalid timestamp");
        res = ResponseHelper::unauthorized(req, "设备时间戳无效");
        return std::nullopt;
    }
    if (std::llabs(static_cast<long long>(std::time(nullptr)) - *clientTs) > 300)
    {
        logDeviceEvent(dbManager, 0, deviceKey, eventType, "failure", requestHash, "timestamp expired");
        res = ResponseHelper::unauthorized(req, "设备时间戳过期");
        return std::nullopt;
    }

    mysqlx::SqlResult deviceResult = dbManager->getSession()
                                          ->sql("SELECT id, hmac_key_cipher FROM attendance_devices WHERE device_key = ? AND is_active = 1 LIMIT 1")
                                          .bind(deviceKey)
                                          .execute();
    mysqlx::Row deviceRow = deviceResult.fetchOne();
    if (!deviceRow)
    {
        logDeviceEvent(dbManager, 0, deviceKey, eventType, "failure", requestHash, "unknown device");
        res = ResponseHelper::unauthorized(req, "设备未授权");
        return std::nullopt;
    }

    DeviceAuth auth;
    auth.deviceId = deviceRow[0].get<int>();
    auth.secret = deviceRow[1].isNull() ? "" : deviceRow[1].get<std::string>();
    auth.deviceKey = deviceKey;
    auth.requestHash = requestHash;
    if (!AttendanceSecurity::verifySignature("POST", canonicalPath, timestamp, nonce, req.body, auth.secret, signature))
    {
        logDeviceEvent(dbManager, auth.deviceId, deviceKey, eventType, "failure", requestHash, "signature mismatch");
        res = ResponseHelper::unauthorized(req, "设备签名无效");
        return std::nullopt;
    }

    // 设备维度限流：全局中间件按 IP 限流，多设备同网关出口时会互相挤兑，
    // 这里按 device_id 再做一层滑动窗口（240 次/分钟，心跳+批量补推足够）。
    // Redis 不可用降级放行，与全局限流同一策略。
    if (RedisClient::instance().enabled())
    {
        const long long nowMs = static_cast<long long>(std::time(nullptr)) * 1000;
        std::optional<RedisClient::WindowHit> hit = RedisClient::instance().zWindowHit(
            "ratelimit:attendance-device:" + std::to_string(auth.deviceId),
            nowMs - 60 * 1000, nowMs, nonce, 61, 240);
        if (hit.has_value() && !hit->admitted)
        {
            logDeviceEvent(dbManager, auth.deviceId, deviceKey, eventType, "failure", requestHash, "device rate limited");
            res = crow::response(429, R"({"error":"Rate limit exceeded","retry_after":60})");
            res.set_header("Content-Type", "application/json");
            return std::nullopt;
        }
    }

    try
    {
        auto *session = dbManager->getSession();
        session->sql("DELETE FROM attendance_device_nonces WHERE expires_at < NOW()").execute();
        session->sql("INSERT INTO attendance_device_nonces (device_id, nonce, expires_at) VALUES (?, ?, DATE_ADD(NOW(), INTERVAL 10 MINUTE))")
            .bind(auth.deviceId)
            .bind(nonce)
            .execute();
    }
    catch (const mysqlx::Error &e)
    {
        // 只有唯一键冲突才是重放；其余 DB 错误按系统错误抛出，不能误判成重放拒掉合法请求。
        const std::string message = e.what();
        if (message.find("Duplicate") == std::string::npos && message.find("duplicate") == std::string::npos)
        {
            throw;
        }
        // 协议约定：nonce 重放（多为设备离线重试）幂等返回 accepted，设备据此清本地缓存；
        // 事实层有 event_id / raw_event_hash 唯一键兜底，不会重复入库。
        logDeviceEvent(dbManager, auth.deviceId, deviceKey, eventType, "failure", requestHash, "replayed nonce");
        auth.duplicateNonce = true;
        return auth;
    }

    dbManager->getSession()
        ->sql("UPDATE attendance_devices SET last_seen_at = NOW() WHERE id = ?")
        .bind(auth.deviceId)
        .execute();

    return auth;
}

std::optional<std::pair<int, std::string>> findStaffByAttendanceNo(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const std::string &attendanceNo)
{
    if (attendanceNo.empty() || !dbManager || !dbManager->getSession())
    {
        return std::nullopt;
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                    ->sql("SELECT id, attendance_no FROM users WHERE attendance_no = ? AND account_type = 'staff' AND is_deleted = 0 LIMIT 1")
                                    .bind(attendanceNo)
                                    .execute();
    mysqlx::Row row = result.fetchOne();
    if (!row)
    {
        return std::nullopt;
    }
    return std::make_pair(row[0].get<int>(), row[1].get<std::string>());
}

std::optional<std::pair<int, std::string>> findStaffByUserId(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId)
{
    if (userId <= 0 || !dbManager || !dbManager->getSession())
    {
        return std::nullopt;
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                    ->sql("SELECT id, COALESCE(attendance_no, '') FROM users WHERE id = ? AND account_type = 'staff' AND is_deleted = 0 LIMIT 1")
                                    .bind(userId)
                                    .execute();
    mysqlx::Row row = result.fetchOne();
    if (!row)
    {
        return std::nullopt;
    }
    return std::make_pair(row[0].get<int>(), row[1].get<std::string>());
}

DevicePunchResult processDevicePunch(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    const DeviceAuth &auth,
    const nlohmann::json &body,
    const std::string &rawEventHash)
{
    if (!body.is_object())
    {
        return {false, "invalid_event", "打卡事件必须是对象"};
    }

    const std::string attendanceNo = jsonString(body, "attendance_no");
    const std::string punchedAtText = jsonString(body, "punched_at");
    const std::string eventId = jsonString(body, "event_id");
    if (attendanceNo.empty() || attendanceNo.size() > 32 || punchedAtText.empty() ||
        (!eventId.empty() && eventId.size() > 128))
    {
        return {false, "invalid_event", "考勤号、打卡时间或事件编号格式无效"};
    }

    std::string verifyMode = jsonString(body, "verify_mode", "unknown");
    if (!isValidVerifyMode(verifyMode))
    {
        verifyMode = "unknown";
    }

    const auto staff = findStaffByAttendanceNo(dbManager, attendanceNo);
    const auto punchedAt = AttendanceService::parseDateTime(punchedAtText);
    if (!staff || !punchedAt)
    {
        return {false, "invalid_staff_or_time", "考勤号或打卡时间无效"};
    }
    if (*punchedAt > std::time(nullptr) + 300)
    {
        return {false, "future_punch", "打卡时间不能超过服务器时间五分钟"};
    }

    Attendance::PunchEvent event;
    event.deviceId = auth.deviceId;
    event.userId = staff->first;
    event.attendanceNo = staff->second;
    event.eventId = eventId;
    event.rawEventHash = rawEventHash;
    event.punchedAt = *punchedAt;
    event.verifyMode = verifyMode;
    event.source = "device";
    if (!AttendanceService::ingestPunch(dbManager, event))
    {
        return {false, "ingest_failed", "打卡写入失败"};
    }

    syncDoctorDutyFromDevicePunch(dbManager, event.userId, event.punchedAt);
    return {true, "accepted", "", event.userId, event.punchedAt};
}

int findPunchUserId(const std::shared_ptr<DatabaseManagerInterface> &dbManager, long long punchId)
{
    if (!dbManager || !dbManager->getSession() || punchId <= 0)
    {
        return 0;
    }
    mysqlx::SqlResult result = dbManager->getSession()
                                    ->sql("SELECT user_id FROM attendance_punches WHERE id = ? LIMIT 1")
                                    .bind(punchId)
                                    .execute();
    mysqlx::Row row = result.fetchOne();
    return row && !row[0].isNull() ? row[0].get<int>() : 0;
}
}

void attendanceRoutes::setupAttendanceRoutes(CrowApp &app, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    static bool routes_setup = false;
    if (routes_setup)
    {
        return;
    }

    CROW_ROUTE(app, "/api/device/attendance/heartbeat")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                try
                {
                    auto auth = authenticateDevice(req, res, dbManager, "/api/device/attendance/heartbeat", "heartbeat");
                    if (!auth)
                    {
                        return;
                    }
                    if (auth->duplicateNonce)
                    {
                        nlohmann::json payload = deviceClockPayload();
                        payload["device_id"] = auth->deviceId;
                        payload["duplicate"] = true;
                        res = ResponseHelper::success(req, payload);
                        return;
                    }
                    logDeviceEvent(dbManager, auth->deviceId, auth->deviceKey, "heartbeat", "success", auth->requestHash, "ok");
                    nlohmann::json payload = deviceClockPayload();
                    payload["device_id"] = auth->deviceId;
                    res = ResponseHelper::success(req, payload);
                }
                catch (const std::exception &)
                {
                    res = ResponseHelper::system_error(req);
                }
            });

    CROW_ROUTE(app, "/api/device/attendance/punch")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                try
                {
                    auto auth = authenticateDevice(req, res, dbManager, "/api/device/attendance/punch", "punch");
                    if (!auth)
                    {
                        return;
                    }
                    if (auth->duplicateNonce)
                    {
                        res = ResponseHelper::success(req, {{"accepted", true}, {"duplicate", true}});
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto bodyOpt = parser.parseJson(req, res);
                    if (!bodyOpt)
                    {
                        logDeviceEvent(dbManager, auth->deviceId, auth->deviceKey, "punch", "failure", auth->requestHash, "invalid json");
                        return;
                    }
                    const DevicePunchResult result = processDevicePunch(dbManager, *auth, *bodyOpt, auth->requestHash);
                    if (!result.accepted)
                    {
                        logDeviceEvent(dbManager, auth->deviceId, auth->deviceKey, "punch", "failure", auth->requestHash, result.code);
                        res = result.code == "ingest_failed"
                                  ? ResponseHelper::operation_failed(req, result.message)
                                  : ResponseHelper::validation(req, result.message);
                        return;
                    }
                    logDeviceEvent(dbManager, auth->deviceId, auth->deviceKey, "punch", "success", auth->requestHash, "ok");
                    res = ResponseHelper::success(req, {{"accepted", true}, {"event_id", jsonString(*bodyOpt, "event_id")}});
                }
                catch (const std::exception &)
                {
                    res = ResponseHelper::system_error(req);
                }
            });

    CROW_ROUTE(app, "/api/device/attendance/punches")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                try
                {
                    auto auth = authenticateDevice(req, res, dbManager, "/api/device/attendance/punches", "punch_batch");
                    if (!auth)
                    {
                        return;
                    }
                    if (auth->duplicateNonce)
                    {
                        res = ResponseHelper::success(req, {{"accepted", true}, {"duplicate", true}});
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto bodyOpt = parser.parseJson(req, res);
                    if (!bodyOpt)
                    {
                        logDeviceEvent(dbManager, auth->deviceId, auth->deviceKey, "punch_batch", "failure", auth->requestHash, "invalid json");
                        return;
                    }
                    if (!bodyOpt->is_object() || !bodyOpt->contains("events") || !(*bodyOpt)["events"].is_array())
                    {
                        res = ResponseHelper::validation(req, "events 必须是数组");
                        return;
                    }

                    const nlohmann::json &events = (*bodyOpt)["events"];
                    if (events.empty() || events.size() > kMaxPunchBatchSize)
                    {
                        res = ResponseHelper::validation(req, "每批打卡事件数量必须为 1 到 100");
                        return;
                    }

                    nlohmann::json results = nlohmann::json::array();
                    int acceptedCount = 0;
                    for (std::size_t index = 0; index < events.size(); ++index)
                    {
                        const nlohmann::json &item = events[index];
                        const std::string itemHash = AttendanceSecurity::sha256Hex(item.dump());
                        const DevicePunchResult result = processDevicePunch(dbManager, *auth, item, itemHash);
                        nlohmann::json itemResult = {
                            {"index", index},
                            {"event_id", item.is_object() ? jsonString(item, "event_id") : ""},
                            {"accepted", result.accepted},
                            {"code", result.code},
                        };
                        if (!result.message.empty())
                        {
                            itemResult["message"] = result.message;
                        }
                        results.push_back(std::move(itemResult));
                        if (result.accepted)
                        {
                            ++acceptedCount;
                            logDeviceEvent(dbManager, auth->deviceId, auth->deviceKey, "punch", "success", itemHash, "batch accepted");
                        }
                        else
                        {
                            logDeviceEvent(dbManager, auth->deviceId, auth->deviceKey, "punch", "failure", itemHash, result.code);
                        }
                    }

                    res = ResponseHelper::success(req, {
                        {"accepted_count", acceptedCount},
                        {"rejected_count", static_cast<int>(events.size()) - acceptedCount},
                        {"results", results},
                    });
                }
                catch (const std::exception &)
                {
                    res = ResponseHelper::system_error(req);
                }
            });

    CROW_ROUTE(app, "/api/device/attendance/person-sync/pull")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                try
                {
                    auto auth = authenticateDevice(req, res, dbManager, "/api/device/attendance/person-sync/pull", "sync_pull");
                    if (!auth)
                    {
                        return;
                    }
                    if (auth->duplicateNonce)
                    {
                        res = ResponseHelper::success(req, {{"tasks", nlohmann::json::array()}, {"duplicate", true}});
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto bodyOpt = parser.parseJson(req, res);
                    if (!bodyOpt || !bodyOpt->is_object())
                    {
                        return;
                    }
                    const int requestedLimit = jsonInt(*bodyOpt, "limit", 50);
                    const int limit = std::max(1, std::min(requestedLimit, kMaxSyncBatchSize));
                    mysqlx::SqlResult queryResult = dbManager->getSession()
                                                        ->sql("SELECT s.id, s.user_id, s.attendance_no, s.desired_state, "
                                                              "COALESCE(u.name, ''), CAST(s.updated_at AS CHAR) "
                                                              "FROM device_person_sync AS s "
                                                              "LEFT JOIN users AS u ON u.id = s.user_id "
                                                              "WHERE s.device_id = ? AND s.sync_state IN ('pending','failed') "
                                                              "ORDER BY CASE WHEN s.sync_state = 'pending' THEN 0 ELSE 1 END, s.updated_at ASC, s.id ASC LIMIT ?")
                                                        .bind(auth->deviceId)
                                                        .bind(limit)
                                                        .execute();
                    nlohmann::json tasks = nlohmann::json::array();
                    for (mysqlx::Row row = queryResult.fetchOne(); row; row = queryResult.fetchOne())
                    {
                        tasks.push_back({
                            {"task_id", row[0].get<long long>()},
                            {"user_id", row[1].get<int>()},
                            {"attendance_no", row[2].get<std::string>()},
                            {"desired_state", row[3].get<std::string>()},
                            {"name", row[4].isNull() ? "" : row[4].get<std::string>()},
                            {"updated_at", row[5].isNull() ? "" : row[5].get<std::string>()},
                        });
                    }
                    logDeviceEvent(dbManager, auth->deviceId, auth->deviceKey, "sync_pull", "success", auth->requestHash, "ok");
                    nlohmann::json payload = deviceClockPayload();
                    payload["tasks"] = std::move(tasks);
                    res = ResponseHelper::success(req, payload);
                }
                catch (const std::exception &)
                {
                    res = ResponseHelper::system_error(req);
                }
            });

    CROW_ROUTE(app, "/api/device/attendance/person-sync/ack")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                try
                {
                    auto auth = authenticateDevice(req, res, dbManager, "/api/device/attendance/person-sync/ack", "sync_ack");
                    if (!auth)
                    {
                        return;
                    }
                    if (auth->duplicateNonce)
                    {
                        res = ResponseHelper::success(req, {{"acknowledged", 0}, {"duplicate", true}});
                        return;
                    }

                    BaseHandler parser(dbManager);
                    auto bodyOpt = parser.parseJson(req, res);
                    if (!bodyOpt || !bodyOpt->is_object() || !bodyOpt->contains("results") || !(*bodyOpt)["results"].is_array())
                    {
                        res = ResponseHelper::validation(req, "results 必须是数组");
                        return;
                    }
                    const nlohmann::json &results = (*bodyOpt)["results"];
                    if (results.empty() || results.size() > kMaxSyncBatchSize)
                    {
                        res = ResponseHelper::validation(req, "每批同步确认数量必须为 1 到 100");
                        return;
                    }

                    int acknowledged = 0;
                    int rejected = 0;
                    for (const nlohmann::json &item : results)
                    {
                        if (!item.is_object() || !item.contains("task_id") || !item["task_id"].is_number_integer() ||
                            !item.contains("success") || !item["success"].is_boolean())
                        {
                            ++rejected;
                            continue;
                        }
                        const long long taskId = item["task_id"].get<long long>();
                        const bool success = item["success"].get<bool>();
                        const std::string message = jsonString(item, "message").substr(0, 255);
                        if (taskId <= 0)
                        {
                            ++rejected;
                            continue;
                        }

                        mysqlx::SqlResult update = dbManager->getSession()
                                                       ->sql("UPDATE device_person_sync SET sync_state = ?, last_error = ?, "
                                                             "last_synced_at = CASE WHEN ? = 1 THEN NOW() ELSE last_synced_at END "
                                                             "WHERE id = ? AND device_id = ?")
                                                       .bind(success ? "synced" : "failed")
                                                       .bind(success ? "" : message)
                                                       .bind(success ? 1 : 0)
                                                       .bind(taskId)
                                                       .bind(auth->deviceId)
                                                       .execute();
                        if (update.getAffectedItemsCount() > 0)
                        {
                            ++acknowledged;
                        }
                        else
                        {
                            ++rejected;
                        }
                    }
                    logDeviceEvent(dbManager, auth->deviceId, auth->deviceKey, "sync_ack", "success", auth->requestHash, "ok");
                    res = ResponseHelper::success(req, {{"acknowledged", acknowledged}, {"rejected", rejected}});
                }
                catch (const std::exception &)
                {
                    res = ResponseHelper::system_error(req);
                }
            });

    CROW_ROUTE(app, "/api/user/attendance/me")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                int userId = isValidUserToken(req, res, dbManager);
                if (res.code != 200 || userId == -1)
                {
                    return;
                }

                const std::string month = queryParam(req, "month");
                res = ResponseHelper::success(req, {{"records", AttendanceService::listMyRecords(dbManager, userId, month)}});
            });

    // GET/POST 必须合并注册：Crow 同一路径重复 CROW_ROUTE 会 "handler already exists" 直接拉不起服务。
    CROW_ROUTE(app, "/api/admin/attendance/devices")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                int operatorUserId = isValidPermissionToken(req, res, dbManager, Permissions::kAttendanceManage);
                if (res.code != 200 || operatorUserId == -1)
                {
                    return;
                }

                if (req.method == crow::HTTPMethod::Get)
                {
                    nlohmann::json devices = nlohmann::json::array();
                    mysqlx::SqlResult result = dbManager->getSession()
                                                ->sql("SELECT id, name, device_key, vendor, location, branch_id, is_active, CAST(last_seen_at AS CHAR), CAST(created_at AS CHAR) "
                                                      "FROM attendance_devices ORDER BY id DESC")
                                                .execute();
                    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
                    {
                        devices.push_back({
                            {"id", row[0].get<int>()},
                            {"name", row[1].isNull() ? "" : row[1].get<std::string>()},
                            {"device_key", row[2].isNull() ? "" : row[2].get<std::string>()},
                            {"vendor", row[3].isNull() ? "" : row[3].get<std::string>()},
                            {"location", row[4].isNull() ? "" : row[4].get<std::string>()},
                            {"branch_id", row[5].isNull() ? nullptr : nlohmann::json(row[5].get<int>())},
                            {"is_active", !row[6].isNull() && row[6].get<int>() == 1},
                            {"last_seen_at", row[7].isNull() ? "" : row[7].get<std::string>()},
                            {"created_at", row[8].isNull() ? "" : row[8].get<std::string>()},
                        });
                    }
                    res = ResponseHelper::success(req, {{"devices", devices}});
                    return;
                }

                BaseHandler parser(dbManager);
                auto bodyOpt = parser.parseJson(req, res);
                if (!bodyOpt)
                {
                    return;
                }
                const nlohmann::json &body = *bodyOpt;
                const std::string name = jsonString(body, "name");
                const std::string deviceKey = jsonString(body, "device_key", randomHex(12));
                const std::string vendor = jsonString(body, "vendor");
                const std::string location = jsonString(body, "location");
                const int branchId = jsonInt(body, "branch_id");
                if (name.empty())
                {
                    res = ResponseHelper::validation(req, "设备名称不能为空");
                    return;
                }

                const std::string secret = randomHex(32);
                mysqlx::SqlResult result = dbManager->getSession()
                                            ->sql("INSERT INTO attendance_devices (name, device_key, hmac_key_cipher, vendor, location, branch_id, created_by) "
                                                  "VALUES (?, ?, ?, ?, ?, ?, ?)")
                                            .bind(name)
                                            .bind(deviceKey)
                                            .bind(secret)
                                            .bind(vendor)
                                            .bind(location)
                                            .bind(branchId > 0 ? mysqlx::Value(branchId) : mysqlx::Value())
                                            .bind(operatorUserId)
                                            .execute();
                const int deviceId = static_cast<int>(result.getAutoIncrementValue());
                // 新设备要能识别现有员工：把在职且有考勤号的人全部补进下发队列（P3 adapter 消费）。
                DevicePersonSync::enqueueDeviceBackfill(dbManager, deviceId);
                res = ResponseHelper::created(req, {
                    {"id", deviceId},
                    {"device_key", deviceKey},
                    {"secret", secret},
                });
                OperationLogger::logUserOperation(
                    dbManager, operatorUserId, "考勤", "创建设备", "成功", "创建设备成功",
                    nlohmann::json({{"device_id", deviceId}, {"device_key", deviceKey}}).dump(),
                    "POST /api/admin/attendance/devices");
            });

    CROW_ROUTE(app, "/api/admin/attendance/devices/<int>/rotate-secret")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int deviceId)
            {
                ResponseEnder ender{res};
                int operatorUserId = isValidPermissionToken(req, res, dbManager, Permissions::kAttendanceManage);
                if (res.code != 200 || operatorUserId == -1)
                {
                    return;
                }

                const std::string secret = randomHex(32);
                mysqlx::SqlResult result = dbManager->getSession()
                                            ->sql("UPDATE attendance_devices SET hmac_key_cipher = ? WHERE id = ?")
                                            .bind(secret)
                                            .bind(deviceId)
                                            .execute();
                if (result.getAffectedItemsCount() == 0)
                {
                    res = ResponseHelper::notFound(req, "设备不存在");
                    return;
                }
                res = ResponseHelper::success(req, {{"device_id", deviceId}, {"secret", secret}});
                OperationLogger::logUserOperation(
                    dbManager, operatorUserId, "考勤", "轮换设备密钥", "成功", "轮换设备密钥成功",
                    nlohmann::json({{"device_id", deviceId}}).dump(),
                    "POST /api/admin/attendance/devices/:id/rotate-secret");
            });

    CROW_ROUTE(app, "/api/admin/attendance/devices/<int>/disable")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res, int deviceId)
            {
                ResponseEnder ender{res};
                int operatorUserId = isValidPermissionToken(req, res, dbManager, Permissions::kAttendanceManage);
                if (res.code != 200 || operatorUserId == -1)
                {
                    return;
                }

                mysqlx::SqlResult result = dbManager->getSession()
                                            ->sql("UPDATE attendance_devices SET is_active = 0 WHERE id = ?")
                                            .bind(deviceId)
                                            .execute();
                if (result.getAffectedItemsCount() == 0)
                {
                    res = ResponseHelper::notFound(req, "设备不存在");
                    return;
                }
                res = ResponseHelper::success(req, {{"device_id", deviceId}, {"is_active", false}});
                OperationLogger::logUserOperation(
                    dbManager, operatorUserId, "考勤", "停用设备", "成功", "停用设备成功",
                    nlohmann::json({{"device_id", deviceId}}).dump(),
                    "POST /api/admin/attendance/devices/:id/disable");
            });

    CROW_ROUTE(app, "/api/admin/attendance/close-day")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                int operatorUserId = isValidPermissionToken(req, res, dbManager, Permissions::kAttendanceManage);
                if (res.code != 200 || operatorUserId == -1)
                {
                    return;
                }

                BaseHandler parser(dbManager);
                auto bodyOpt = parser.parseJson(req, res);
                if (!bodyOpt)
                {
                    return;
                }
                const std::string workDate = jsonString(*bodyOpt, "work_date");
                if (workDate.empty())
                {
                    res = ResponseHelper::validation(req, "work_date 不能为空");
                    return;
                }
                const int inserted = AttendanceService::closeDay(dbManager, workDate);
                res = ResponseHelper::success(req, {{"work_date", workDate}, {"absent_records_created", inserted}});
                OperationLogger::logUserOperation(
                    dbManager, operatorUserId, "考勤", "日终关账", "成功", "日终关账完成",
                    nlohmann::json({{"work_date", workDate}, {"absent_records_created", inserted}}).dump(),
                    "POST /api/admin/attendance/close-day");
            });

    CROW_ROUTE(app, "/api/admin/attendance/records")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                int operatorUserId = isValidPermissionToken(req, res, dbManager, Permissions::kAttendanceRead);
                if (res.code != 200 || operatorUserId == -1)
                {
                    return;
                }

                nlohmann::json query = {
                    {"month", queryParam(req, "month")},
                    {"start_date", queryParam(req, "start_date")},
                    {"end_date", queryParam(req, "end_date")},
                    {"status", queryParam(req, "status")},
                };
                const std::string userIdText = queryParam(req, "user_id");
                if (!userIdText.empty())
                {
                    try
                    {
                        query["user_id"] = std::stoi(userIdText);
                    }
                    catch (const std::exception &)
                    {
                        res = ResponseHelper::validation(req, "user_id 必须是整数");
                        return;
                    }
                }
                const std::string pageText = queryParam(req, "page");
                const std::string pageSizeText = queryParam(req, "page_size");
                try
                {
                    if (!pageText.empty())
                    {
                        query["page"] = std::stoi(pageText);
                    }
                    if (!pageSizeText.empty())
                    {
                        query["page_size"] = std::stoi(pageSizeText);
                    }
                }
                catch (const std::exception &)
                {
                    res = ResponseHelper::validation(req, "page 和 page_size 必须是整数");
                    return;
                }
                res = ResponseHelper::success(req, AttendanceService::listRecords(dbManager, operatorUserId, query));
            });

    CROW_ROUTE(app, "/api/admin/attendance/manual-punch")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                int operatorUserId = isValidPermissionToken(req, res, dbManager, Permissions::kAttendanceManage);
                if (res.code != 200 || operatorUserId == -1)
                {
                    return;
                }

                BaseHandler parser(dbManager);
                auto bodyOpt = parser.parseJson(req, res);
                if (!bodyOpt)
                {
                    return;
                }
                const nlohmann::json &body = *bodyOpt;
                const int targetUserId = jsonInt(body, "user_id");
                const std::string punchedAtText = jsonString(body, "punched_at");
                const std::string reason = jsonString(body, "reason");
                std::string verifyMode = jsonString(body, "verify_mode", "manual");
                if (!isValidVerifyMode(verifyMode))
                {
                    verifyMode = "manual";
                }

                auto staff = findStaffByUserId(dbManager, targetUserId);
                auto punchedAt = AttendanceService::parseDateTime(punchedAtText);
                if (!staff || !punchedAt || reason.empty())
                {
                    res = ResponseHelper::validation(req, "员工、补卡时间和原因不能为空");
                    return;
                }
                if (!targetWithinOrgScope(dbManager, operatorUserId, staff->first))
                {
                    res = ResponseHelper::notFound(req, "员工不存在");
                    return;
                }

                Attendance::PunchEvent event;
                event.userId = staff->first;
                event.attendanceNo = staff->second;
                event.eventId = "manual-" + AttendanceSecurity::sha256Hex(std::to_string(operatorUserId) + "|" + std::to_string(targetUserId) + "|" + punchedAtText).substr(0, 32);
                event.rawEventHash = AttendanceSecurity::sha256Hex(req.body);
                event.punchedAt = *punchedAt;
                event.verifyMode = verifyMode;
                event.source = "manual";
                event.createdBy = operatorUserId;
                event.correctionNote = reason;

                if (!AttendanceService::ingestPunch(dbManager, event))
                {
                    res = ResponseHelper::operation_failed(req, "补卡失败");
                    return;
                }
                // 设计 §8：手动补卡与设备打卡走同一个医生在线态联动（函数内部只认当天事件）。
                syncDoctorDutyFromDevicePunch(dbManager, event.userId, event.punchedAt);
                res = ResponseHelper::success(req, {{"accepted", true}});
                OperationLogger::logUserOperation(
                    dbManager, operatorUserId, "考勤", "手动补卡", "成功", "手动补卡成功",
                    nlohmann::json({{"target_user_id", targetUserId}, {"punched_at", punchedAtText}, {"reason", reason}}).dump(),
                    "POST /api/admin/attendance/manual-punch");
            });

    CROW_ROUTE(app, "/api/admin/attendance/void-punch")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                int operatorUserId = isValidPermissionToken(req, res, dbManager, Permissions::kAttendanceManage);
                if (res.code != 200 || operatorUserId == -1)
                {
                    return;
                }

                BaseHandler parser(dbManager);
                auto bodyOpt = parser.parseJson(req, res);
                if (!bodyOpt)
                {
                    return;
                }
                const long long punchId = jsonLongLong(*bodyOpt, "punch_id");
                const std::string reason = jsonString(*bodyOpt, "reason");
                if (punchId <= 0 || reason.empty())
                {
                    res = ResponseHelper::validation(req, "punch_id 和 reason 不能为空");
                    return;
                }
                const int targetUserId = findPunchUserId(dbManager, punchId);
                if (targetUserId <= 0 || !targetWithinOrgScope(dbManager, operatorUserId, targetUserId))
                {
                    res = ResponseHelper::notFound(req, "打卡记录不存在");
                    return;
                }

                if (!AttendanceService::voidPunch(dbManager, punchId, operatorUserId, reason))
                {
                    res = ResponseHelper::notFound(req, "打卡记录不存在或已作废");
                    return;
                }
                res = ResponseHelper::success(req, {{"voided", true}});
                OperationLogger::logUserOperation(
                    dbManager, operatorUserId, "考勤", "作废打卡", "成功", "作废打卡成功",
                    nlohmann::json({{"punch_id", punchId}, {"target_user_id", targetUserId}, {"reason", reason}}).dump(),
                    "POST /api/admin/attendance/void-punch");
            });

    // 打卡明细：作废的前置——UI 必须先看到某人某天的具体 punch 才能定位 punch_id（设计 §15.5）。
    CROW_ROUTE(app, "/api/admin/attendance/punches")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                int operatorUserId = isValidPermissionToken(req, res, dbManager, Permissions::kAttendanceRead);
                if (res.code != 200 || operatorUserId == -1)
                {
                    return;
                }

                int targetUserId = 0;
                try
                {
                    targetUserId = std::stoi(queryParam(req, "user_id"));
                }
                catch (const std::exception &)
                {
                    res = ResponseHelper::validation(req, "user_id 必须是整数");
                    return;
                }
                const std::string workDate = queryParam(req, "date");
                if (targetUserId <= 0 || workDate.empty())
                {
                    res = ResponseHelper::validation(req, "user_id 和 date 不能为空");
                    return;
                }
                if (!targetWithinOrgScope(dbManager, operatorUserId, targetUserId))
                {
                    res = ResponseHelper::notFound(req, "员工不存在");
                    return;
                }

                nlohmann::json punches = nlohmann::json::array();
                mysqlx::SqlResult result = dbManager->getSession()
                                            ->sql("SELECT p.id, p.device_id, COALESCE(d.name, ''), CAST(p.punched_at AS CHAR), "
                                                  "p.verify_mode, p.source, p.is_voided, p.void_reason, p.created_by "
                                                  "FROM attendance_punches AS p "
                                                  "LEFT JOIN attendance_devices AS d ON d.id = p.device_id "
                                                  "WHERE p.user_id = ? AND DATE(p.punched_at) = ? ORDER BY p.punched_at ASC")
                                            .bind(targetUserId)
                                            .bind(workDate)
                                            .execute();
                for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
                {
                    punches.push_back({
                        {"id", row[0].get<int64_t>()},
                        {"device_id", row[1].isNull() ? nullptr : nlohmann::json(row[1].get<int>())},
                        {"device_name", row[2].isNull() ? "" : row[2].get<std::string>()},
                        {"punched_at", row[3].isNull() ? "" : row[3].get<std::string>()},
                        {"verify_mode", row[4].isNull() ? "" : row[4].get<std::string>()},
                        {"source", row[5].isNull() ? "" : row[5].get<std::string>()},
                        {"is_voided", !row[6].isNull() && row[6].get<int>() == 1},
                        {"void_reason", row[7].isNull() ? "" : row[7].get<std::string>()},
                        {"created_by", row[8].isNull() ? nullptr : nlohmann::json(row[8].get<int>())},
                    });
                }
                res = ResponseHelper::success(req, {{"user_id", targetUserId}, {"date", workDate}, {"punches", punches}});
            });

    // 工作日维护：closeDay 只对配置为工作日的日期补 absent，没有写入口整条日结链路就是死的。
    CROW_ROUTE(app, "/api/admin/attendance/workdays")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post, crow::HTTPMethod::Options)(
            [dbManager](const crow::request &req, crow::response &res)
            {
                ResponseEnder ender{res};
                int operatorUserId = isValidPermissionToken(req, res, dbManager, Permissions::kAttendanceManage);
                if (res.code != 200 || operatorUserId == -1)
                {
                    return;
                }

                if (req.method == crow::HTTPMethod::Get)
                {
                    const std::string month = queryParam(req, "month");
                    nlohmann::json workdays = nlohmann::json::array();
                    auto query = dbManager->getSession()
                                     ->sql("SELECT CAST(work_date AS CHAR), day_type, CAST(check_in_start AS CHAR), CAST(check_in_end AS CHAR), "
                                           "CAST(check_out_start AS CHAR), CAST(check_out_end AS CHAR), note "
                                           "FROM attendance_workdays WHERE DATE_FORMAT(work_date, '%Y-%m') = ? ORDER BY work_date ASC");
                    mysqlx::SqlResult result = query.bind(month).execute();
                    for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
                    {
                        workdays.push_back({
                            {"work_date", row[0].isNull() ? "" : row[0].get<std::string>()},
                            {"day_type", row[1].isNull() ? "" : row[1].get<std::string>()},
                            {"check_in_start", row[2].isNull() ? "" : row[2].get<std::string>()},
                            {"check_in_end", row[3].isNull() ? "" : row[3].get<std::string>()},
                            {"check_out_start", row[4].isNull() ? "" : row[4].get<std::string>()},
                            {"check_out_end", row[5].isNull() ? "" : row[5].get<std::string>()},
                            {"note", row[6].isNull() ? "" : row[6].get<std::string>()},
                        });
                    }
                    res = ResponseHelper::success(req, {{"month", month}, {"workdays", workdays}});
                    return;
                }

                BaseHandler parser(dbManager);
                auto bodyOpt = parser.parseJson(req, res);
                if (!bodyOpt)
                {
                    return;
                }
                const nlohmann::json &body = *bodyOpt;
                const std::string workDate = jsonString(body, "work_date");
                const std::string dayType = jsonString(body, "day_type", "workday");
                if (workDate.empty() || (dayType != "workday" && dayType != "holiday"))
                {
                    res = ResponseHelper::validation(req, "work_date 不能为空，day_type 必须是 workday 或 holiday");
                    return;
                }
                const std::string checkInStart = jsonString(body, "check_in_start", "08:00:00");
                const std::string checkInEnd = jsonString(body, "check_in_end", "09:00:00");
                const std::string checkOutStart = jsonString(body, "check_out_start", "18:00:00");
                const std::string checkOutEnd = jsonString(body, "check_out_end", "23:59:59");
                const std::string note = jsonString(body, "note");

                dbManager->getSession()
                    ->sql("INSERT INTO attendance_workdays (work_date, day_type, check_in_start, check_in_end, check_out_start, check_out_end, note, created_by) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
                          "ON DUPLICATE KEY UPDATE day_type = VALUES(day_type), check_in_start = VALUES(check_in_start), "
                          "check_in_end = VALUES(check_in_end), check_out_start = VALUES(check_out_start), "
                          "check_out_end = VALUES(check_out_end), note = VALUES(note)")
                    .bind(workDate)
                    .bind(dayType)
                    .bind(checkInStart)
                    .bind(checkInEnd)
                    .bind(checkOutStart)
                    .bind(checkOutEnd)
                    .bind(note)
                    .bind(operatorUserId)
                    .execute();
                res = ResponseHelper::success(req, {{"work_date", workDate}, {"day_type", dayType}});
                OperationLogger::logUserOperation(
                    dbManager, operatorUserId, "考勤", "维护工作日", "成功", "维护工作日成功",
                    nlohmann::json({{"work_date", workDate}, {"day_type", dayType}}).dump(),
                    "POST /api/admin/attendance/workdays");
            });

    routes_setup = true;
}
