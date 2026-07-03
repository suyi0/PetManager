#include "userHandler.h"
#include "../../../../utils/AuthIdentifierUtils.h"
#include "../userPhoneSync/userPhoneSync.h"
#include "../../../../services/auth/AuthSessionStore.h"
#include "../../../../services/auth/AuthLoginFailureStore.h"
#include "../../../../services/redis/RedisClient.h"
#include "../../../../services/redis/redisLock/RedisLock.h"
#include "../../../../services/redis/doctorListCache/DoctorListCache.h"
#include "../../../../utils/requestUtils/RequestUtils.h"
#include "../../../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../../../services/realtime/doctorBroadcaster/doctorQueueBroadcaster.h"
#include "../../../../services/realtime/doctorListBroadcaster/doctorListBroadcaster.h"
#include "roleTypeUtils/roleTypeUtils.h"
#include "statusLabelUtils/StatusLabelUtils.h"
#include <vector>

// userHandler 预约域：创建/取消/删除预约、医生列表、到院等。
// 从 userHandler.cpp 按域拆出；类声明仍在 userHandler.h。

namespace
{
    // 获取今天日期，格式（YYYY-MM-DD）
    std::string getTodayDate()
    {
        const boost::posix_time::ptime currentDateTime = boost::posix_time::second_clock::local_time();
        return formatDateOnly(currentDateTime);
    }

    nlohmann::json buildDoctorJson(const mysqlx::Row &row)
    {
        nlohmann::json doctor;
        doctor["doctor_id"] = row[0].isNull() ? 0 : row[0].get<int>();
        doctor["id"] = doctor["doctor_id"];
        doctor["name"] = row[1].isNull() ? "" : row[1].get<std::string>();
        doctor["phone"] = row[2].isNull() ? "" : row[2].get<std::string>();
        doctor["email"] = row[3].isNull() ? "" : row[3].get<std::string>();
        doctor["specialty"] = row[4].isNull() ? "" : row[4].get<std::string>();
        doctor["status"] = row[5].isNull() ? "offline" : row[5].get<std::string>();
        return doctor;
    }
}

crow::response userHandler::createReservation(const crow::request &req, int user_id, int pet_id, int doctor_id, std::string reservation_type, std::string date, std::string time_slot, std::string status)
{
    try
    {
        // 检查数据库连接是否存在
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "创建预约", "database connection failed", user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        // 检查必要字段是否存在
        if (!StatusLabelUtils::isValidReservationStatus(status))
        {
            return ResponseHelper::validation(req, "预约状态不合法");
        }
        const std::string dbStatus = StatusLabelUtils::toDbReservationStatus(status);

        if (reservation_type.size() > 30)
        {
            return ResponseHelper::validation(req, "预约类型不能超过30个字符");
        }

        if (user_id > 0 && pet_id > 0 && doctor_id > 0 && !reservation_type.empty() && !date.empty() && !time_slot.empty())
        {
            try
            {
                mysqlx::SqlResult petResult = dbManager->getSession()
                                                  ->sql("SELECT COUNT(*) FROM pets WHERE id = ? AND user_id = ? AND is_deleted = 0")
                                                  .bind(pet_id, user_id)
                                                  .execute();
                auto petRow = petResult.fetchOne();
                if (!petRow || petRow[0].get<int>() == 0)
                {
                    return ResponseHelper::validation(req, "宠物不存在或不属于当前用户");
                }

                // 下单时从 DB 校验医生：必须是未删除的医生角色；且**当预约的是今天**时，医生须在岗(online)。
                // 这才是真正的守门人——不依赖前端显示是否新鲜；医生下班后即便列表还旧，当天预约这里也会拒掉。
                // 注意：系统支持预约未来 7 天，而 onlineDoctors 是"当日签到"概念，未来日期本就没有行；
                // 所以 online 只对当天预约要求，未来日期只校验"是合法医生"，否则会误拒全部未来预约。
                const int doctorRoleId = RoleTypeUtils::getRoleId(dbManager, "医生");
                if (doctorRoleId <= 0)
                {
                    return ResponseHelper::system_error(req, "医生角色不存在");
                }
                mysqlx::SqlResult doctorResult = dbManager->getSession()
                                                     ->sql("SELECT COALESCE(od.status, 'offline') "
                                                           "FROM users AS u "
                                                           "LEFT JOIN onlineDoctors AS od "
                                                           "ON od.doctor_id = u.id AND od.date = ? "
                                                           "WHERE u.id = ? AND u.type_id = ? AND u.is_deleted = 0 "
                                                           "LIMIT 1")
                                                     .bind(date, doctor_id, doctorRoleId)
                                                     .execute();
                auto doctorRow = doctorResult.fetchOne();
                if (!doctorRow)
                {
                    return ResponseHelper::validation(req, "医生不存在或不可预约");
                }
                if (date == getTodayDate() && doctorRow[0].get<std::string>() != "online")
                {
                    return ResponseHelper::validation(req, "该医生当前不在岗，暂不可预约");
                }

                // 槽位短锁：现有"查重后插入"在多实例并发 / 双击下可能两笔都查不到、都插入，
                // 导致同医生同时段被重复预约。这里用 Redis 锁把同一槽位的创建串行化，缩小竞态窗口。
                // 注意：这是配角，真正的 durable 修复是给 reservations 加"有效预约唯一约束"（见交接说明）。
                // Redis 不可用时退回原有 DB 查重逻辑，不阻断下单。
                const std::string slotLockKey =
                    "reservation:slot:" + std::to_string(doctor_id) + ":" + date + ":" + time_slot;
                RedisLockGuard slotLock; // token 安全；析构自动释放（仅当仍是自己持有）
                if (RedisClient::instance().enabled())
                {
                    RedisLock::Result res = RedisLock::tryAcquire(slotLockKey, 30);
                    if (res.outcome == RedisLock::Outcome::Contended)
                    {
                        // 明确被占用：同槽位另一笔预约正在处理 → 拒绝。
                        return ResponseHelper::validation(req, "该医生当前时间段已被预约");
                    }
                    // Acquired→持锁；Unavailable(Redis 出错)→不持锁，继续走下面的 DB 查重 + 唯一约束兜底，不误拒。
                    slotLock = std::move(res.guard);
                }

                mysqlx::SqlResult slotResult = dbManager->getSession()
                                                   ->sql("SELECT id FROM reservations "
                                                         "WHERE doctor_id = ? AND date = ? AND time_slot = ? "
                                                         "AND COALESCE(status, 'scheduled') NOT IN ('cancelled', 'failed') "
                                                         "AND is_deleted = 0 "
                                                         "LIMIT 1")
                                                   .bind(doctor_id, date, time_slot)
                                                   .execute();
                if (slotResult.fetchOne())
                {
                    return ResponseHelper::validation(req, "该医生当前时间段已被预约");
                }

                mysqlx::SqlResult insertResult = dbManager->getSession()
                                                     ->sql("INSERT INTO reservations (user_id, pet_id, doctor_id, reservation_type, date, time_slot, status) "
                                                           "VALUES (?, ?, ?, ?, ?, ?, ?)")
                                                     .bind(user_id, pet_id, doctor_id, reservation_type, date, time_slot, dbStatus)
                                                     .execute();

                uint64_t reservationId = insertResult.getAutoIncrementValue();

                mysqlx::SqlResult createdResult = dbManager->getSession()
                                                      ->sql("SELECT r.id, r.user_id, r.pet_id, r.doctor_id, COALESCE(p.pet_name, ''), "
                                                            "COALESCE(r.reservation_type, ''), CAST(r.date AS CHAR), COALESCE(r.time_slot, ''), COALESCE(r.status, ''), "
                                                            "CAST(r.created_at AS CHAR) "
                                                            "FROM reservations AS r "
                                                            "LEFT JOIN pets AS p ON r.pet_id = p.id "
                                                            "WHERE r.id = ? LIMIT 1")
                                                      .bind(reservationId)
                                                      .execute();
                auto createdRow = createdResult.fetchOne();

                nlohmann::json response;
                response["reservation_status"] = StatusLabelUtils::toDisplayReservationStatus(dbStatus);
                response["message"] = "预约成功";

                if (createdRow)
                {
                    const std::string createdReservationType = createdRow[5].isNull() ? "" : createdRow[5].get<std::string>();
                    const std::string createdDate = createdRow[6].isNull() ? "" : createdRow[6].get<std::string>();
                    const std::string createdSlot = createdRow[7].isNull() ? "" : createdRow[7].get<std::string>();
                    response["id"] = createdRow[0].get<int>();
                    response["user_id"] = createdRow[1].get<int>();
                    response["pet_id"] = createdRow[2].get<int>();
                    response["doctor_id"] = createdRow[3].get<int>();
                    response["pet_name"] = createdRow[4].isNull() ? "" : createdRow[4].get<std::string>();
                    response["reservation_type"] = createdReservationType;
                    response["reservationType"] = createdReservationType;
                    response["date"] = createdDate;
                    response["time_slot"] = createdSlot;
                    response["status"] = StatusLabelUtils::toDisplayReservationStatus(createdRow[8].isNull() ? dbStatus : createdRow[8].get<std::string>());
                    response["created_at"] = createdRow[9].isNull() ? "" : createdRow[9].get<std::string>();
                    response["price"] = 0;
                }

                return ResponseHelper::success(req, response);
            }
            catch (const mysqlx::Error &e)
            {
                // 命中"有效预约唯一约束"(DB 层 uq_active_slot)的重复键 → 并发竞态的输家。
                // 这让 DB 唯一约束成为真正的守门人：即便 Redis 槽位锁失效(Redis 不可用或锁过期)，
                // 并发下也只有一笔 INSERT 成功，另一笔在此被识别为"已被预约"，而不是抛 500。
                if (std::string(e.what()).find("Duplicate") != std::string::npos)
                {
                    return ResponseHelper::validation(req, "该医生当前时间段已被预约");
                }
                std::cerr << "Database error: " << e.what() << std::endl;
                OperationLogger::LogExceptionOperation(dbManager, req, "预约", "创建预约", e.what(), user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
                return ResponseHelper::database_error(req, "Failed to create reservation", e.what());
            }
        }
        else
        {
            return ResponseHelper::validation(req, "Missing required fields");
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "创建预约", e.what(), user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to save reservation", e.what());
    }
}

// 获取预约时间表数据接口
nlohmann::json userHandler::getReservationDate()
{
    // 这里应该生成并返回时间表
    Reservate r;
    auto schedule = r.generateSchedule();
    return schedule;
}

// 获取医生列表接口
crow::response userHandler::getDoctorList(const crow::request &req)
{
    if (!checkDbConnection())
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取医生列表", "database connection failed");
        return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
    }

    try
    {
        const int doctorRoleId = RoleTypeUtils::getRoleId(dbManager, "医生");
        if (doctorRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "医生角色不存在");
        }

        const std::string todayDate = getTodayDate();

        nlohmann::json doctorList = DoctorListCache::cachedDoctorList(
            todayDate, [this, &todayDate, doctorRoleId]()
            {
                mysqlx::RowResult result = dbManager->getSession()
                                               ->sql("SELECT u.id, u.name, p.phone, u.email, u.user_specialty, "
                                                     "COALESCE(od.status, 'offline') "
                                                     "FROM users AS u "
                                                     "LEFT JOIN phones AS p ON p.user_id = u.id "
                                                     "LEFT JOIN onlineDoctors AS od "
                                                     "ON od.doctor_id = u.id AND od.date = ? "
                                                     "WHERE u.type_id = ? AND u.is_deleted = 0")
                                               .bind(todayDate, doctorRoleId)
                                               .execute();

                nlohmann::json list = nlohmann::json::array();
                for (const auto &row : result)
                {
                    list.push_back(buildDoctorJson(row));
                }
                return list;
            });

        return ResponseHelper::success(req, doctorList);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取医生列表", e.what());
        return ResponseHelper::operation_failed(req, "Failed to fetch doctor list", e.what());
    }
}

// 取消预约接口
crow::response userHandler::cancelReservation(const crow::request &req, int userId, int reservationId)
{
    try
    {
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "取消预约", "database connection failed", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        if (userId <= 0 || reservationId <= 0)
        {
            return ResponseHelper::unauthorized(req, "缺少权限验证结果/订单ID");
        }

        std::string status = "cancelled";

        // 验证用户和预约记录是否匹配
        mysqlx::SqlResult reservation_result = dbManager->getSession()
                                                   ->sql("SELECT user_id FROM reservations WHERE id = ? AND is_deleted = 0")
                                                   .bind(reservationId)
                                                   .execute();

        auto reservation_row = reservation_result.fetchOne();
        if (!reservation_row)
        {
            return ResponseHelper::notFound(req, "Reservation record does not exist");
        }

        if (reservation_row[0].get<int>() == userId) // 操作用户和预约记录用户匹配
        {

            mysqlx::SqlResult result = dbManager->getSession()
                                           ->sql("UPDATE reservations SET status = ? WHERE id = ?")
                                           .bind(status, reservationId)
                                           .execute();

            // 检查是否有记录被更新
            if (result.getAffectedItemsCount() > 0)
            {
                // 返回成功响应
                nlohmann::json response;
                response["message"] = "取消成功";
                response["reservation_id"] = reservationId;
                response["status"] = StatusLabelUtils::toDisplayReservationStatus(status);
                return ResponseHelper::success(req, response);
            }
            else
            {
                return ResponseHelper::notFound(req, "未找到指定的预约记录");
            }
        }
        else // 操作用户和预约记录用户不匹配
        {
            return ResponseHelper::permission_denied(
                req,
                "预约记录不匹配",
                "Reservation record does not belong to current user");
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "取消预约", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to cancel reservation", e.what());
    }
}

// 删除预约记录接口
crow::response userHandler::deleteReservation(const crow::request &req, int userId, int reservationId)
{
    try
    {
        // 检查数据库连接是否存在
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "删除预约记录", "database connection failed", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        if (userId <= 0 || reservationId <= 0)
        {
            return ResponseHelper::unauthorized(req, "缺少权限验证结果/订单ID");
        }

        mysqlx::SqlResult reservation_result = dbManager->getSession()
                                                   ->sql("SELECT user_id FROM reservations WHERE id = ?")
                                                   .bind(reservationId)
                                                   .execute();

        auto reservation_row = reservation_result.fetchOne();

        if (!reservation_row)
        {
            return ResponseHelper::notFound(req, "Reservation record does not exist");
        }

        if (reservation_row[0].get<int>() == userId) // 操作用户和预约记录用户匹配才允许删除
        {

            // 用户侧删除采用软删除，历史记录仍保留在数据库中。
            mysqlx::SqlResult result = dbManager->getSession()
                                           ->sql("UPDATE reservations "
                                                 "SET is_deleted = 1, deleted_at = NOW(), deleted_by = ? "
                                                 "WHERE id = ? AND user_id = ? AND is_deleted = 0")
                                           .bind(userId, reservationId, userId)
                                           .execute();

            // 检查是否有记录被删除
            if (result.getAffectedItemsCount() > 0)
            {
                // 返回成功响应
                nlohmann::json response;
                response["message"] = "预约记录删除成功";
                response["reservation_id"] = reservationId;
                return ResponseHelper::success(req, response);
            }
            else
            {
                return ResponseHelper::notFound(req, "未找到指定的预约记录");
            }
        }
        else
        {
            return ResponseHelper::permission_denied(
                req,
                "预约记录不匹配",
                "Reservation record does not belong to current user");
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "删除预约记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to delete reservation", e.what());
    }
}

crow::response userHandler::toTheHospital(const crow::request &req, int userId, int reservationId)
{
    try
    {
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "预约到院", "database connection failed", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        if (userId <= 0 || reservationId <= 0)
        {
            return ResponseHelper::unauthorized(req, "缺少权限验证结果/订单ID");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            mysqlx::SqlResult reservationResult = session->sql("SELECT id, user_id, doctor_id, pet_id, reservation_type, CAST(date AS CHAR), COALESCE(time_slot, '') "
                                                               "FROM reservations "
                                                               "WHERE id = ? AND user_id = ? AND is_deleted = 0 AND status = 'scheduled' "
                                                               "LIMIT 1 FOR UPDATE")
                                                      .bind(reservationId, userId)
                                                      .execute();
            auto reservationRow = reservationResult.fetchOne();
            if (!reservationRow)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::notFound(req, "未找到可到院签到的预约记录");
            }

            const int ownerId = reservationRow[1].isNull() ? 0 : reservationRow[1].get<int>();
            const int doctorId = reservationRow[2].isNull() ? 0 : reservationRow[2].get<int>();
            const int petId = reservationRow[3].isNull() ? 0 : reservationRow[3].get<int>();
            const std::string reservationDate = reservationRow[5].isNull() ? "" : reservationRow[5].get<std::string>();
            const std::string timeSlot = reservationRow[6].isNull() ? "" : reservationRow[6].get<std::string>();
            
            if (ownerId != userId)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::permission_denied(req, "预约记录不匹配", "预约记录不属于当前用户");
            }
            if (doctorId <= 0 || petId <= 0)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::validation(req, "预约记录缺少医生或宠物信息");
            }
            if (reservationDate.empty())
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::validation(req, "预约记录日期数据错误");
            }
            
            const std::string queueDate = getTodayDate();
            std::string scheduledAt = reservationDate + " 00:00:00";
            // 获取预约时间格式（YYYY-MM-DD HH:mm:ss）
            if (timeSlot.size() >= 5 && timeSlot[2] == ':')
            {
                scheduledAt = reservationDate + " " + timeSlot.substr(0, 5) + ":00";
            }

            // 同一天的队列编号由计数表生成，避免并发签到时 MAX(queue_number)+1 撞号。
            session->sql("INSERT INTO medicalQueueCounters(queue_date, current_number) "
                         "SELECT ?, COALESCE(MAX(CAST(queue_number AS UNSIGNED)), 0) "
                         "FROM medicalQueues "
                         "WHERE queue_date = ? "
                         "ON DUPLICATE KEY UPDATE current_number = current_number")
                .bind(queueDate, queueDate)
                .execute();

            // 更新 medicalQueueCounters.current_number = LAST_INSERT_ID(current_number + 1)
            session->sql("UPDATE medicalQueueCounters "
                         "SET current_number = LAST_INSERT_ID(current_number + 1) "
                         "WHERE queue_date = ?")
                .bind(queueDate)
                .execute();

            // SELECT LAST_INSERT_ID() 返回 当前连接最近一次​ 由以下方式产生的值：
            // AUTO_INCREMENT / LAST_INSERT_ID(expr)
            mysqlx::SqlResult queueNumberResult = session->sql("SELECT LAST_INSERT_ID()").execute();

            auto queueNumberRow = queueNumberResult.fetchOne();
            // 获取下一个待就诊队列编号
            const int nextQueueNumber = queueNumberRow && !queueNumberRow[0].isNull() ? queueNumberRow[0].get<int>() : 0;
            if (nextQueueNumber <= 0)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::operation_failed(req, "Failed to create queue number", "未能生成待就诊队列编号");
            }
            const std::string queueNumber = std::to_string(nextQueueNumber);

            // 创建待就诊队列
            mysqlx::SqlResult queueInsertResult = session->sql("INSERT INTO medicalQueues(queue_date, queue_number, doctor_id, pet_id, owner_id, status, source, triage_level, scheduled_at, arrived_at) "
                                                               "VALUES (?, ?, ?, ?, ?, 'waiting', 'appointment', 'normal', ?, ?)")
                                                      .bind(queueDate, queueNumber, doctorId, petId, userId, scheduledAt, getCreateTime())
                                                      .execute();

            if (queueInsertResult.getAffectedItemsCount() == 0)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::operation_failed(req, "Failed to create medical queue", "未能创建待就诊队列记录");
            }

            // 更新预约记录表软删除和到院状态
            mysqlx::SqlResult reservationUpdateResult = session->sql("UPDATE reservations "
                                                                     "SET status = 'arrived', is_deleted = 1, deleted_at = NOW(), deleted_by = ? "
                                                                     "WHERE id = ? AND user_id = ? AND is_deleted = 0 AND status = 'scheduled'")
                                                            .bind(userId, reservationId, userId)
                                                            .execute();

            if (reservationUpdateResult.getAffectedItemsCount() == 0)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::operation_failed(req, "Failed to update reservation status", "未能更新预约到院状态");
            }

            session->sql("COMMIT").execute();
            DoctorQueueBroadcaster::instance().notifyQueueChanged(doctorId);

            return ResponseHelper::success(req, "签到成功");
        }
        catch (const std::exception &e)
        {
            rollbackTransactionQuietly(*session);
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "预约到院", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
            return ResponseHelper::operation_failed(req, "Failed to to the hospital", e.what());
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "预约到院", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to to the hospital", e.what());
    }
}
