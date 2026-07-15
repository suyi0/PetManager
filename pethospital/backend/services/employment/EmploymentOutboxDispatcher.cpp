#include "EmploymentOutboxDispatcher.h"

#include "../attendance/DevicePersonSync.h"
#include "../auth/AccessRevocation.h"
#include "../realtime/doctorListBroadcaster/doctorListBroadcaster.h"
#include "../redis/doctorListCache/DoctorListCache.h"
#include "../../utils/Utils.h"

#include <iostream>
#include <nlohmann/json.hpp>

namespace EmploymentOutboxDispatcher
{
namespace
{
// processing 租约：短于此视为 recent（拒绝二次 claim），超过可 stale 重试。
constexpr int kProcessingLeaseSeconds = 120;

bool applyPayload(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int userId,
    const std::string &eventType,
    const nlohmann::json &payload)
{
    AccessRevocation::onUserAccessChanged(userId);

    const bool separated = eventType == "employment_separated" ||
                           payload.value("account_type", "") == "customer";
    if (separated)
    {
        DevicePersonSync::enqueueRemove(dbManager, userId);
    }
    else
    {
        DevicePersonSync::enqueueUpsert(dbManager, userId);
    }

    const std::string oldKind = payload.value("old_staff_kind", "");
    const std::string newKind = payload.value("new_staff_kind", "");
    if (oldKind == "doctor" || newKind == "doctor")
    {
        DoctorListCache::invalidateDoctorList();
        DoctorListBroadcaster::instance().notifyDoctorListChanged();
    }
    return true;
}

void clearLeaseAndSetStatus(
    mysqlx::Session &session,
    long long outboxId,
    const std::string &status,
    const std::string &lastError = "")
{
    if (status == "completed")
    {
        session.sql("UPDATE employment_event_outbox "
                    "SET status = 'completed', completed_at = NOW(), last_error = '', "
                    "locked_at = NULL "
                    "WHERE id = ?")
            .bind(outboxId)
            .execute();
    }
    else if (status == "failed")
    {
        session.sql("UPDATE employment_event_outbox "
                    "SET status = 'failed', last_error = ?, locked_at = NULL, "
                    "next_attempt_at = DATE_ADD(NOW(), INTERVAL LEAST(attempts, 30) MINUTE) "
                    "WHERE id = ?")
            .bind(lastError.substr(0, 1000), outboxId)
            .execute();
    }
}
}

bool dispatchOne(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    long long outboxId)
{
    if (!dbManager || !dbManager->getSession() || outboxId <= 0)
    {
        return false;
    }

    auto session = dbManager->getSession();
    int userId = 0;
    std::string eventType;
    std::string payloadRaw = "{}";

    session->sql("START TRANSACTION").execute();
    try
    {
        mysqlx::Row row = session->sql(
                                     "SELECT id, user_id, event_type, payload, status, attempts, "
                                     "locked_at, "
                                     "CASE WHEN locked_at IS NULL THEN 1 "
                                     "     WHEN locked_at < DATE_SUB(NOW(), INTERVAL ? SECOND) THEN 1 "
                                     "     ELSE 0 END AS lease_expired "
                                     "FROM employment_event_outbox WHERE id = ? LIMIT 1 FOR UPDATE")
                              .bind(kProcessingLeaseSeconds, outboxId)
                              .execute()
                              .fetchOne();
        if (!row)
        {
            rollbackTransactionQuietly(*session);
            return false;
        }

        const std::string status = row[4].isNull() ? "" : row[4].get<std::string>();
        if (status == "completed")
        {
            session->sql("COMMIT").execute();
            return true;
        }

        const int leaseExpired = row[7].isNull() ? 1 : row[7].get<int>();
        // recent processing：另一 worker 持有租约，拒绝二次 claim
        if (status == "processing" && leaseExpired == 0)
        {
            session->sql("COMMIT").execute();
            return false;
        }
        // pending / failed / stale processing 可 claim
        if (status != "pending" && status != "failed" && !(status == "processing" && leaseExpired == 1))
        {
            session->sql("COMMIT").execute();
            return false;
        }

        auto claim = session->sql(
                                "UPDATE employment_event_outbox "
                                "SET status = 'processing', attempts = attempts + 1, "
                                "locked_at = NOW(), last_error = '' "
                                "WHERE id = ? AND ("
                                "  status IN ('pending','failed') OR "
                                "  (status = 'processing' AND "
                                "   (locked_at IS NULL OR locked_at < DATE_SUB(NOW(), INTERVAL ? SECOND)))"
                                ")")
                         .bind(outboxId, kProcessingLeaseSeconds)
                         .execute();
        if (claim.getAffectedItemsCount() != 1)
        {
            // 并发 claim 失败
            session->sql("COMMIT").execute();
            return false;
        }

        userId = row[1].get<int>();
        eventType = row[2].isNull() ? "" : row[2].get<std::string>();
        payloadRaw = row[3].isNull() ? "{}" : row[3].get<std::string>();
        session->sql("COMMIT").execute();
    }
    catch (const std::exception &e)
    {
        rollbackTransactionQuietly(*session);
        std::cerr << "EmploymentOutboxDispatcher claim failed: " << e.what() << std::endl;
        return false;
    }

    nlohmann::json payload = nlohmann::json::object();
    try
    {
        payload = nlohmann::json::parse(payloadRaw);
    }
    catch (...)
    {
        payload = nlohmann::json::object();
    }

    try
    {
        applyPayload(dbManager, userId, eventType, payload);
        clearLeaseAndSetStatus(*session, outboxId, "completed");
        return true;
    }
    catch (const std::exception &e)
    {
        clearLeaseAndSetStatus(*session, outboxId, "failed", e.what());
        std::cerr << "EmploymentOutboxDispatcher failed id=" << outboxId << ": " << e.what() << std::endl;
        return false;
    }
}

int dispatchPending(
    const std::shared_ptr<DatabaseManagerInterface> &dbManager,
    int limit)
{
    if (!dbManager || !dbManager->getSession() || limit <= 0)
    {
        return 0;
    }
    int done = 0;
    try
    {
        // pending / failed（到期）/ stale processing
        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT id FROM employment_event_outbox "
                                             "WHERE ("
                                             "  (status IN ('pending','failed') "
                                             "   AND (next_attempt_at IS NULL OR next_attempt_at <= NOW())) "
                                             "  OR (status = 'processing' "
                                             "      AND (locked_at IS NULL "
                                             "          OR locked_at < DATE_SUB(NOW(), INTERVAL ? SECOND)))"
                                             ") "
                                             "ORDER BY id ASC LIMIT ?")
                                       .bind(kProcessingLeaseSeconds, limit)
                                       .execute();
        for (mysqlx::Row row = result.fetchOne(); row; row = result.fetchOne())
        {
            if (!row[0].isNull() && dispatchOne(dbManager, row[0].get<int64_t>()))
            {
                ++done;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "EmploymentOutboxDispatcher::dispatchPending: " << e.what() << std::endl;
    }
    return done;
}
}
