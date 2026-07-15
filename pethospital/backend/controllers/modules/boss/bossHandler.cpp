#include <cmath>
#include <vector>
#include "bossHandler.h"
#include "../../../services/employment/CompensationWorkflowService.h"
#include "../../../services/employment/EmploymentAssignmentService.h"
#include "../../../utils/requestUtils/RequestUtils.h"

namespace
{
    constexpr const char *kTotalHolder = "总份额";
    constexpr const char *kRemainingHolder = "剩余份额";
    constexpr const char *kDecisionShareType = "决策股分布图";
    constexpr const char *kDividendShareType = "分红股分布图";

    bool isReservedHolder(const std::string &holder)
    {
        return holder == kTotalHolder || holder == kRemainingHolder;
    }

    std::string resolveStockItemType(const std::string &holder)
    {
        if (holder == kTotalHolder)
        {
            return "total";
        }
        if (holder == kRemainingHolder)
        {
            return "remaining";
        }
        return "holder";
    }

    nlohmann::json buildStockDistribution(mysqlx::Session &session, const std::string &shareType)
    {
        mysqlx::SqlResult result = session.sql("SELECT id, holder, share FROM stock WHERE share_type = ? ORDER BY id ASC")
                                       .bind(shareType)
                                       .execute();

        struct StockRow
        {
            int id;
            std::string holder;
            long long share;
        };

        std::vector<StockRow> rows;
        rows.reserve(8);

        long long totalShare = 0LL;
        for (const auto &row : result)
        {
            const int id = row[0].isNull() ? -1 : row[0].get<int>();
            const std::string holder = row[1].isNull() ? "" : row[1].get<std::string>();
            const long long share = row[2].isNull() ? 0LL : row[2].get<long long>();

            if (holder == kTotalHolder)
            {
                totalShare = share;
            }

            rows.push_back({id, holder, share});
        }

        nlohmann::json stocks = nlohmann::json::array();
        if (totalShare <= 0)
        {
            return stocks;
        }

        for (const auto &row : rows)
        {
            const double rawPercentage = static_cast<double>(row.share) / static_cast<double>(totalShare) * 100.0;
            const double percentage = std::round(rawPercentage * 100.0) / 100.0;

            stocks.push_back({
                {"id", row.id},
                {"type", resolveStockItemType(row.holder)},
                {"holder", row.holder},
                {"share", row.share},
                {"percentage", percentage},
            });
        }

        return stocks;
    }
} // namespace

crow::response bossHandler::allocateTotalStock(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
        {
            return res;
        }
        auto &request_body = request_body_opt.value();

        const std::string shareType = clean_string(request_body.value("share_type", ""));
        const long long share = request_body.value("share", -1LL);

        if (shareType.empty() || share <= 0)
        {
            return ResponseHelper::validation(req, "股份类型或总份额参数无效");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            mysqlx::Row totalRow = session->sql("SELECT id, share FROM stock WHERE holder = ? AND share_type = ? LIMIT 1 FOR UPDATE")
                                       .bind(kTotalHolder, shareType)
                                       .execute()
                                       .fetchOne();

            const bool hasTotalRow = static_cast<bool>(totalRow);
            const int totalId = hasTotalRow ? totalRow[0].get<int>() : -1;
            const long long oldTotalShare = hasTotalRow && !totalRow[1].isNull() ? totalRow[1].get<long long>() : 0LL;

            mysqlx::SqlResult totalResult = hasTotalRow
                                                ? session->sql("UPDATE stock SET share = ? WHERE id = ?")
                                                      .bind(share, totalId)
                                                      .execute()
                                                : session->sql("INSERT INTO stock (share_type, holder, share) VALUES (?, ?, ?)")
                                                      .bind(shareType, kTotalHolder, share)
                                                      .execute();

            if (totalResult.getAffectedItemsCount() != 1)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::operation_failed(req, shareType + "总份额分配失败");
            }

            mysqlx::Row remainingRow = session->sql("SELECT id, share FROM stock WHERE holder = ? AND share_type = ? LIMIT 1 FOR UPDATE")
                                           .bind(kRemainingHolder, shareType)
                                           .execute()
                                           .fetchOne();

            const bool hasRemainingRow = static_cast<bool>(remainingRow);
            const long long oldRemainingShare = hasRemainingRow && !remainingRow[1].isNull() ? remainingRow[1].get<long long>() : 0LL;

            mysqlx::SqlResult remainingResult;
            if (hasRemainingRow)
            {
                const long long newRemainingShare = oldRemainingShare + (share - oldTotalShare);
                if (newRemainingShare < 0)
                {
                    rollbackTransactionQuietly(*session);
                    return ResponseHelper::validation(req, "总份额不能小于已分配份额");
                }

                remainingResult = session->sql("UPDATE stock SET share = ? WHERE id = ?")
                                      .bind(newRemainingShare, remainingRow[0].get<int>())
                                      .execute();
            }
            else
            {
                remainingResult = session->sql("INSERT INTO stock (share_type, holder, share) VALUES (?, ?, ?)")
                                      .bind(shareType, kRemainingHolder, share)
                                      .execute();
            }

            if (remainingResult.getAffectedItemsCount() != 1)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::operation_failed(req, shareType + "剩余份额分配失败");
            }

            session->sql("COMMIT").execute();
            return ResponseHelper::success(req, shareType + "总份额分配成功");
        }
        catch (...)
        {
            rollbackTransactionQuietly(*session);
            throw;
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response bossHandler::allocateStock(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
        {
            return res;
        }
        auto &request_body = request_body_opt.value();

        const std::string shareType = clean_string(request_body.value("share_type", ""));
        const std::string holder = clean_string(request_body.value("holder", ""));
        const long long share = request_body.value("share", -1LL);

        if (shareType.empty() || holder.empty() || share <= 0)
        {
            return ResponseHelper::validation(req, "缺失股份类型、股东名称或股份份额信息");
        }

        if (isReservedHolder(holder))
        {
            return ResponseHelper::validation(req, "系统保留记录不允许直接修改");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            mysqlx::Row remainingRow = session->sql("SELECT id, share FROM stock "
                                                    "WHERE holder = ? AND share_type = ? "
                                                    "LIMIT 1 FOR UPDATE")
                                           .bind(kRemainingHolder, shareType)
                                           .execute()
                                           .fetchOne();

            if (!remainingRow)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::unavailable(req, "未初始化" + shareType + "剩余股份数据");
            }

            const int remainingId = remainingRow[0].get<int>();
            const long long remainingShare = remainingRow[1].isNull() ? 0LL : remainingRow[1].get<long long>();
            if (remainingShare < share)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::validation(req, "剩余股份不足，无法继续分配");
            }

            mysqlx::SqlResult insertResult = session->sql("INSERT INTO stock (share_type, holder, share) VALUES (?, ?, ?)")
                                                 .bind(shareType, holder, share)
                                                 .execute();
            if (insertResult.getAffectedItemsCount() != 1)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::operation_failed(req, "新增股东股份失败");
            }

            mysqlx::SqlResult updateRemainingResult = session->sql("UPDATE stock SET share = share - ? WHERE id = ? AND share_type = ? AND share >= ?")
                                                          .bind(share, remainingId, shareType, share)
                                                          .execute();
            if (updateRemainingResult.getAffectedItemsCount() != 1)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::operation_failed(req, "扣减" + shareType + "剩余股份失败");
            }

            session->sql("COMMIT").execute();
            return ResponseHelper::success(req, "新增" + shareType + "股东成功");
        }
        catch (...)
        {
            rollbackTransactionQuietly(*session);
            throw;
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response bossHandler::changeStock(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
        {
            return res;
        }
        auto &request_body = request_body_opt.value();

        const int holderId = request_body.value("holderId", -1);
        const std::string shareType = clean_string(request_body.value("share_type", ""));
        const std::string holder = clean_string(request_body.value("holder", ""));
        const long long share = request_body.value("share", -1LL);

        if (holderId <= 0 || shareType.empty() || holder.empty() || share <= 0)
        {
            return ResponseHelper::validation(req, "缺失股东ID、股份类型、股份拥有者或者股份份额信息");
        }

        if (isReservedHolder(holder))
        {
            return ResponseHelper::validation(req, "系统保留记录不允许直接修改");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            mysqlx::Row holderRow = session->sql("SELECT holder, share FROM stock WHERE id = ? AND share_type = ? LIMIT 1 FOR UPDATE")
                                        .bind(holderId, shareType)
                                        .execute()
                                        .fetchOne();

            if (!holderRow)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::notFound(req, "该股东不存在");
            }

            const std::string oldHolder = holderRow[0].get<std::string>();
            if (isReservedHolder(oldHolder))
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::validation(req, "系统保留记录不允许直接修改");
            }

            const long long oldShare = holderRow[1].isNull() ? 0LL : holderRow[1].get<long long>();
            const long long delta = share - oldShare;

            if (oldHolder == holder && delta == 0)
            {
                session->sql("COMMIT").execute();
                return ResponseHelper::success(req, "未有需要变动的股东信息");
            }

            int remainingId = -1;
            if (delta != 0)
            {
                mysqlx::Row remainingRow = session->sql("SELECT id, share FROM stock "
                                                        "WHERE holder = ? AND share_type = ? "
                                                        "LIMIT 1 FOR UPDATE")
                                               .bind(kRemainingHolder, shareType)
                                               .execute()
                                               .fetchOne();

                if (!remainingRow)
                {
                    rollbackTransactionQuietly(*session);
                    return ResponseHelper::unavailable(req, "未初始化剩余股份数据");
                }

                remainingId = remainingRow[0].get<int>();
                const long long remainingShare = remainingRow[1].isNull() ? 0LL : remainingRow[1].get<long long>();
                if (delta > 0 && remainingShare < delta)
                {
                    rollbackTransactionQuietly(*session);
                    return ResponseHelper::validation(req, "剩余股份不足，无法继续分配");
                }
            }

            mysqlx::SqlResult changeHolderResult = session->sql("UPDATE stock SET holder = ?, share = ? WHERE id = ? AND share_type = ?")
                                                       .bind(holder, share, holderId, shareType)
                                                       .execute();
            if (changeHolderResult.getAffectedItemsCount() != 1)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::operation_failed(req, "更新股东失败");
            }

            if (delta != 0)
            {
                mysqlx::SqlResult updateRemainingResult = session->sql("UPDATE stock SET share = share - ? WHERE id = ? AND share_type = ?")
                                                              .bind(delta, remainingId, shareType)
                                                              .execute();

                if (updateRemainingResult.getAffectedItemsCount() != 1)
                {
                    rollbackTransactionQuietly(*session);
                    return ResponseHelper::operation_failed(req, "更新剩余股份失败");
                }
            }

            session->sql("COMMIT").execute();
            return ResponseHelper::success(req, "更新股东成功");
        }
        catch (...)
        {
            rollbackTransactionQuietly(*session);
            throw;
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response bossHandler::getStock(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        auto session = dbManager->getSession();
        nlohmann::json response;
        response["decisionStocks"] = buildStockDistribution(*session, kDecisionShareType);
        response["dividendStocks"] = buildStockDistribution(*session, kDividendShareType);

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

namespace
{
crow::response decisionResultToResponse(
    const crow::request &req,
    const EmploymentAssignmentService::DecisionResult &result)
{
    if (!result.ok)
    {
        if (result.httpStatus == 404)
        {
            return ResponseHelper::notFound(req, result.message);
        }
        if (result.httpStatus == 403)
        {
            return ResponseHelper::permission_denied(req, result.message, result.errorCode);
        }
        if (result.httpStatus == 409)
        {
            return ResponseHelper::fail(
                req, 409, ResponseCode::BusinessConflict, result.message,
                ResponseErrorType::BusinessConflict,
                result.errorCode.empty() ? result.message : result.errorCode);
        }
        if (result.httpStatus >= 500)
        {
            return ResponseHelper::system_error(req, result.message);
        }
        return ResponseHelper::validation(req, result.message);
    }

    return ResponseHelper::success(req, {
        {"request_id", result.requestId},
        {"employment_id", result.employmentId},
        {"assignment_status", result.assignmentStatus},
        {"action", result.decisionAction},
        {"message", result.message},
    });
}
}

crow::response bossHandler::listEmploymentAssignmentApprovals(
    const crow::request &req,
    int operatorUserId)
{
    try
    {
        EmploymentAssignmentService::ListRequestsQuery query;
        query.operatorUserId = operatorUserId;

        if (const char *status = req.url_params.get("status"))
        {
            query.status = status;
        }
        int page = 1;
        int pageSize = 20;
        if (const char *pageRaw = req.url_params.get("page"))
        {
            try
            {
                page = std::stoi(pageRaw);
            }
            catch (...)
            {
            }
        }
        if (const char *sizeRaw = req.url_params.get("pageSize"))
        {
            try
            {
                pageSize = std::stoi(sizeRaw);
            }
            catch (...)
            {
            }
        }
        query.page = RequestUtils::normalizePage(page);
        query.pageSize = RequestUtils::normalizePageSize(pageSize, 20, 100);

        const auto result = EmploymentAssignmentService::listRequests(dbManager, query);
        if (!result.ok)
        {
            if (result.httpStatus == 403)
            {
                return ResponseHelper::permission_denied(req, result.message, result.errorCode);
            }
            if (result.httpStatus >= 500)
            {
                return ResponseHelper::system_error(req, result.message);
            }
            return ResponseHelper::validation(req, result.message);
        }

        return ResponseHelper::success(req, {
            {"items", result.items},
            {"total", result.total},
            {"page", result.page},
            {"pageSize", result.pageSize},
        });
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response bossHandler::decideEmploymentAssignmentApproval(
    const crow::request &req,
    int operatorUserId,
    long long requestId,
    const nlohmann::json &body)
{
    try
    {
        if (requestId <= 0)
        {
            return ResponseHelper::notFound(req, "申请不存在");
        }

        // decision body 必须显式包含 action=approve|reject、reason、expectedRowVersion；
        // 不得从字段缺失或布尔值推断。
        if (!body.contains("action") || body["action"].is_null() ||
            (body["action"].is_string() && body["action"].get<std::string>().empty()))
        {
            return ResponseHelper::validation(req, "action 必填");
        }
        if (!body["action"].is_string())
        {
            return ResponseHelper::validation(req, "action 取值不合法");
        }
        const std::string actionStr = body["action"].get<std::string>();

        EmploymentAssignmentService::DecisionRequest decisionReq;
        decisionReq.operatorUserId = operatorUserId;
        decisionReq.requestId = requestId;
        if (actionStr == "approve")
        {
            decisionReq.action = EmploymentAssignmentService::DecisionAction::Approve;
        }
        else if (actionStr == "reject")
        {
            decisionReq.action = EmploymentAssignmentService::DecisionAction::Reject;
        }
        else
        {
            // 未知动作 fail closed
            return ResponseHelper::validation(req, "action 取值不合法");
        }

        decisionReq.reason = RequestUtils::getJsonString(body, "reason");
        if (decisionReq.reason.empty())
        {
            return ResponseHelper::validation(req, "reason 不能为空");
        }

        if (!body.contains("expectedRowVersion") || body["expectedRowVersion"].is_null())
        {
            return ResponseHelper::validation(req, "expectedRowVersion 必填");
        }
        if (!body["expectedRowVersion"].is_number_integer())
        {
            return ResponseHelper::validation(req, "expectedRowVersion 必须是整数");
        }
        decisionReq.expectedRowVersion = body["expectedRowVersion"].get<int>();
        decisionReq.hasExpectedRowVersion = true;

        const auto result = EmploymentAssignmentService::decide(dbManager, decisionReq);
        return decisionResultToResponse(req, result);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

namespace
{
crow::response compensationOpToResponse(
    const crow::request &req,
    const CompensationWorkflowService::OpResult &result)
{
    if (!result.ok)
    {
        if (result.httpStatus == 404)
        {
            return ResponseHelper::notFound(req, result.message);
        }
        if (result.httpStatus == 403)
        {
            return ResponseHelper::permission_denied(req, result.message, result.errorCode);
        }
        if (result.httpStatus == 409)
        {
            return ResponseHelper::fail(
                req, 409, ResponseCode::BusinessConflict, result.message,
                ResponseErrorType::BusinessConflict,
                result.errorCode.empty() ? result.message : result.errorCode);
        }
        if (result.httpStatus >= 500)
        {
            return ResponseHelper::system_error(req, result.message);
        }
        return ResponseHelper::validation(req, result.message);
    }
    return ResponseHelper::success(req, result.data);
}
}

crow::response bossHandler::listCompensationApprovals(
    const crow::request &req,
    int operatorUserId)
{
    try
    {
        CompensationWorkflowService::ListQuery query;
        query.operatorUserId = operatorUserId;
        query.audience = "boss";
        if (const char *status = req.url_params.get("status"))
        {
            query.status = status;
        }
        else
        {
            query.status = "submitted";
        }
        int page = 1;
        int pageSize = 20;
        if (const char *pageRaw = req.url_params.get("page"))
        {
            try
            {
                page = std::stoi(pageRaw);
            }
            catch (...)
            {
            }
        }
        if (const char *sizeRaw = req.url_params.get("pageSize"))
        {
            try
            {
                pageSize = std::stoi(sizeRaw);
            }
            catch (...)
            {
            }
        }
        query.page = RequestUtils::normalizePage(page);
        query.pageSize = RequestUtils::normalizePageSize(pageSize, 20, 100);

        const auto result = CompensationWorkflowService::listProposals(dbManager, query);
        if (!result.ok)
        {
            if (result.httpStatus == 403)
            {
                return ResponseHelper::permission_denied(req, result.message, result.errorCode);
            }
            if (result.httpStatus >= 500)
            {
                return ResponseHelper::system_error(req, result.message);
            }
            return ResponseHelper::validation(req, result.message);
        }
        return ResponseHelper::success(req, {
            {"items", result.items},
            {"total", result.total},
            {"page", result.page},
            {"pageSize", result.pageSize},
        });
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response bossHandler::decideCompensationApproval(
    const crow::request &req,
    int operatorUserId,
    long long proposalId,
    const nlohmann::json &body)
{
    try
    {
        if (proposalId <= 0)
        {
            return ResponseHelper::notFound(req, "提案不存在");
        }
        if (!body.contains("action") || body["action"].is_null() ||
            (body["action"].is_string() && body["action"].get<std::string>().empty()))
        {
            return ResponseHelper::validation(req, "action 必填");
        }
        if (!body["action"].is_string())
        {
            return ResponseHelper::validation(req, "action 取值不合法");
        }
        const std::string actionStr = body["action"].get<std::string>();

        CompensationWorkflowService::DecisionRequest decisionReq;
        decisionReq.operatorUserId = operatorUserId;
        decisionReq.proposalId = proposalId;
        if (actionStr == "approve")
        {
            decisionReq.action = CompensationWorkflowService::DecisionAction::Approve;
        }
        else if (actionStr == "return")
        {
            decisionReq.action = CompensationWorkflowService::DecisionAction::Return;
        }
        else
        {
            // 未知动作 fail closed
            return ResponseHelper::validation(req, "action 取值不合法");
        }

        decisionReq.reason = RequestUtils::getJsonString(body, "reason");
        if (decisionReq.reason.empty())
        {
            return ResponseHelper::validation(req, "reason 不能为空");
        }
        if (!body.contains("expectedRowVersion") || body["expectedRowVersion"].is_null())
        {
            return ResponseHelper::validation(req, "expectedRowVersion 必填");
        }
        if (!body["expectedRowVersion"].is_number_integer())
        {
            return ResponseHelper::validation(req, "expectedRowVersion 必须是整数");
        }
        decisionReq.expectedRowVersion = body["expectedRowVersion"].get<int>();
        decisionReq.hasExpectedRowVersion = true;

        const auto result = CompensationWorkflowService::decideProposal(dbManager, decisionReq);
        return compensationOpToResponse(req, result);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}
