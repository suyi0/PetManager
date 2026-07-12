#include "financeHandler.h"
#include "../../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../../services/realtime/financeBroadcaster/financeHomeDataBroadcaster.h"
#include "../../../services/rbac/RbacService.h"
#include "../../../utils/permissions/Permissions.h"
#include "../../../utils/requestUtils/RequestUtils.h"
#include <cmath>
#include <optional>
#include <sstream>

namespace
{
    // 请求参数解析工具统一来自 RequestUtils（原本 finance/admin/warehouseManager 各复制一份）。
    using RequestUtils::getJsonInt;
    using RequestUtils::getJsonString;
    using RequestUtils::normalizePage;
    using RequestUtils::normalizePageSize;

    // 获取当天的开始和结束时间字符串，格式为 "YYYY-MM-DD HH:MM:SS"
    std::pair<std::string, std::string> getTodayRange()
    {
        const boost::posix_time::ptime now =
            boost::posix_time::second_clock::local_time();
        const boost::posix_time::ptime dayStart(now.date());
        const boost::posix_time::ptime nextDayStart =
            dayStart + boost::gregorian::days(1);

        return {formatDateTime(dayStart), formatDateTime(nextDayStart)};
    }

    std::optional<int> currentRequestUserId(const crow::request &req)
    {
        const std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
        {
            return std::nullopt;
        }
        auto claims = JwtUtils::getTokenClaims(authHeader.substr(7));
        if (!claims || claims->userId <= 0)
        {
            return std::nullopt;
        }
        return claims->userId;
    }

    std::string joinIds(const std::vector<int> &ids)
    {
        std::ostringstream stream;
        for (std::size_t i = 0; i < ids.size(); ++i)
        {
            if (i > 0)
            {
                stream << ",";
            }
            stream << ids[i];
        }
        return stream.str();
    }

    std::string orgScopeCondition(
        const crow::request &req,
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        const std::string &departmentColumn)
    {
        const std::optional<int> userId = currentRequestUserId(req);
        if (!userId.has_value())
        {
            return " AND 1 = 0 ";
        }

        // EffectiveOrgScope 显式区分「不限制(scope:all)」与「无可见部门」，
        // 查库异常/客户账户不会被误判为全量可见（fail-closed）
        const RbacService::EffectiveOrgScope scope =
            RbacService::loadEffectiveOrgScope(dbManager, userId.value());
        if (scope.unrestricted)
        {
            return "";
        }
        if (scope.departmentIds.empty())
        {
            return " AND 1 = 0 ";
        }
        return " AND " + departmentColumn + " IN (" + joinIds(scope.departmentIds) + ") ";
    }

    std::string orgScopeConditionForUser(
        int userId,
        const std::shared_ptr<DatabaseManagerInterface> &dbManager,
        const std::string &departmentColumn)
    {
        if (userId <= 0)
        {
            return " AND 1 = 0 ";
        }

        const RbacService::EffectiveOrgScope scope =
            RbacService::loadEffectiveOrgScope(dbManager, userId);
        if (scope.unrestricted)
        {
            return "";
        }
        if (scope.departmentIds.empty())
        {
            return " AND 1 = 0 ";
        }
        return " AND " + departmentColumn + " IN (" + joinIds(scope.departmentIds) + ") ";
    }

    int ensureCurrentPayrollPeriod(mysqlx::Session &session)
    {
        session.sql("INSERT INTO payrollPeriod (payroll_month, status, version_no) "
                    "SELECT DATE_FORMAT(CURDATE(), '%Y-%m-01'), 'first_review', 1 "
                    "FROM DUAL WHERE NOT EXISTS ("
                    "SELECT 1 FROM payrollPeriod WHERE payroll_month = DATE_FORMAT(CURDATE(), '%Y-%m-01') "
                    "AND status IN ('calculating','first_review','second_review'))")
            .execute();
        auto row = session.sql("SELECT id FROM payrollPeriod "
                               "WHERE payroll_month = DATE_FORMAT(CURDATE(), '%Y-%m-01') "
                               "ORDER BY version_no DESC LIMIT 1")
                       .execute()
                       .fetchOne();
        if (!row || row[0].isNull())
        {
            throw std::runtime_error("无法创建当前工资周期");
        }
        return row[0].get<int>();
    }
}

double financeHandler::calculateSalesCount()
{
    try
    {
        if (!checkDbConnection())
        {
            return -1;
        }

        const auto [dayStart, nextDayStart] = getTodayRange();

        const double salesCount = dbManager->getSession()
                                      ->sql("SELECT COALESCE(ROUND(SUM(order_totalprice)), 0) "
                                            "FROM orders "
                                            "WHERE created_at >= ? AND created_at < ?")
                                      .bind(dayStart)
                                      .bind(nextDayStart)
                                      .execute()
                                      .fetchOne()[0]
                                      .get<double>();

        return salesCount;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to get salesCount: " + std::string(e.what()));
    }
}

double financeHandler::calculateCostCount()
{
    try
    {
        if (!checkDbConnection())
        {
            return -1;
        }

        const auto [dayStart, nextDayStart] = getTodayRange();

        const double employeeCostCount = dbManager->getSession()
                                             ->sql("SELECT COALESCE(ROUND(SUM(s.total_salary / 31)), 0) "
                                                   "FROM salary AS s "
                                                   "JOIN users AS u ON u.id = s.user_id "
                                                   "WHERE u.account_type = 'staff'")
                                             .execute()
                                             .fetchOne()[0]
                                             .get<double>();

        const double itemCostCount = dbManager->getSession()
                                         ->sql("SELECT COALESCE(ROUND(SUM(om.total_price)), 0) "
                                               "FROM orderMedicines AS om "
                                               "JOIN orders AS o ON om.order_id = o.id "
                                               "WHERE o.created_at >= ? AND o.created_at < ?")
                                         .bind(dayStart)
                                         .bind(nextDayStart)
                                         .execute()
                                         .fetchOne()[0]
                                         .get<double>();

        const double costCount = employeeCostCount + itemCostCount;

        return costCount;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to get costCount: " + std::string(e.what()));
    }
}

double financeHandler::calculateCostCount(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return -1;
        }

        const auto [dayStart, nextDayStart] = getTodayRange();
        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");

        const double employeeCostCount = dbManager->getSession()
                                             ->sql("SELECT COALESCE(ROUND(SUM(s.total_salary / 31)), 0) "
                                                   "FROM salary AS s "
                                                   "JOIN users AS u ON u.id = s.user_id "
                                                   "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                                   "WHERE u.account_type = 'staff' " +
                                                   scopeFilter)
                                             .execute()
                                             .fetchOne()[0]
                                             .get<double>();

        const double itemCostCount = dbManager->getSession()
                                         ->sql("SELECT COALESCE(ROUND(SUM(om.total_price)), 0) "
                                               "FROM orderMedicines AS om "
                                               "JOIN orders AS o ON om.order_id = o.id "
                                               "WHERE o.created_at >= ? AND o.created_at < ?")
                                         .bind(dayStart)
                                         .bind(nextDayStart)
                                         .execute()
                                         .fetchOne()[0]
                                         .get<double>();

        return employeeCostCount + itemCostCount;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to get scoped costCount: " + std::string(e.what()));
    }
}

double financeHandler::calculateCostCount(int userId)
{
    try
    {
        if (!checkDbConnection())
        {
            return -1;
        }

        const auto [dayStart, nextDayStart] = getTodayRange();
        const std::string scopeFilter = orgScopeConditionForUser(userId, dbManager, "pos.department_id");

        const double employeeCostCount = dbManager->getSession()
                                             ->sql("SELECT COALESCE(ROUND(SUM(s.total_salary / 31)), 0) "
                                                   "FROM salary AS s "
                                                   "JOIN users AS u ON u.id = s.user_id "
                                                   "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                                   "WHERE u.account_type = 'staff' " +
                                                   scopeFilter)
                                             .execute()
                                             .fetchOne()[0]
                                             .get<double>();

        const double itemCostCount = dbManager->getSession()
                                         ->sql("SELECT COALESCE(ROUND(SUM(om.total_price)), 0) "
                                               "FROM orderMedicines AS om "
                                               "JOIN orders AS o ON om.order_id = o.id "
                                               "WHERE o.created_at >= ? AND o.created_at < ?")
                                         .bind(dayStart)
                                         .bind(nextDayStart)
                                         .execute()
                                         .fetchOne()[0]
                                         .get<double>();

        return employeeCostCount + itemCostCount;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Failed to get scoped costCount: " + std::string(e.what()));
    }
}

nlohmann::json financeHandler::buildHomeData()
{
    if (!checkDbConnection())
    {
        throw std::runtime_error("Database connection failed");
    }

    // 获取每日员工工资开销记录
    mysqlx::SqlResult dailyExpensesResult = dbManager->getSession()
                                                ->sql("SELECT COALESCE(ROUND(SUM(total_salary / 31)), 0)  "
                                                      "FROM salary ")
                                                .execute();

    auto dailyRow = dailyExpensesResult.fetchOne();

    double salesCount = calculateSalesCount();
    double costCount = calculateCostCount();

    return {
        {"dailyExpense", dailyRow[0].isNull() ? 0.0 : dailyRow[0].get<double>()},
        {"dailyCost", costCount},
        {"dailySales", salesCount},
        {"dailyProfit", salesCount - costCount}};
}

nlohmann::json financeHandler::buildHomeData(const crow::request &req)
{
    if (!checkDbConnection())
    {
        throw std::runtime_error("Database connection failed");
    }

    const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");
    mysqlx::SqlResult dailyExpensesResult = dbManager->getSession()
                                                ->sql("SELECT COALESCE(ROUND(SUM(s.total_salary / 31)), 0) "
                                                      "FROM salary AS s "
                                                      "JOIN users AS u ON u.id = s.user_id "
                                                      "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                                      "WHERE u.account_type = 'staff' " +
                                                      scopeFilter)
                                                .execute();

    auto dailyRow = dailyExpensesResult.fetchOne();

    double salesCount = calculateSalesCount();
    double costCount = calculateCostCount(req);

    return {
        {"dailyExpense", dailyRow && !dailyRow[0].isNull() ? dailyRow[0].get<double>() : 0.0},
        {"dailyCost", costCount},
        {"dailySales", salesCount},
        {"dailyProfit", salesCount - costCount}};
}

nlohmann::json financeHandler::buildHomeData(int userId)
{
    if (!checkDbConnection())
    {
        throw std::runtime_error("Database connection failed");
    }

    const std::string scopeFilter = orgScopeConditionForUser(userId, dbManager, "pos.department_id");
    mysqlx::SqlResult dailyExpensesResult = dbManager->getSession()
                                                ->sql("SELECT COALESCE(ROUND(SUM(s.total_salary / 31)), 0) "
                                                      "FROM salary AS s "
                                                      "JOIN users AS u ON u.id = s.user_id "
                                                      "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                                      "WHERE u.account_type = 'staff' " +
                                                      scopeFilter)
                                                .execute();

    auto dailyRow = dailyExpensesResult.fetchOne();

    double salesCount = calculateSalesCount();
    double costCount = calculateCostCount(userId);

    return {
        {"dailyExpense", dailyRow && !dailyRow[0].isNull() ? dailyRow[0].get<double>() : 0.0},
        {"dailyCost", costCount},
        {"dailySales", salesCount},
        {"dailyProfit", salesCount - costCount}};
}

crow::response financeHandler::getHomeData(const crow::request &req)
{
    try
    {
        return ResponseHelper::success(req, buildHomeData(req));
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response financeHandler::updateEmployeeSalary(const crow::request &req, int goalUserId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        if (goalUserId <= 0)
        {
            return ResponseHelper::error(req, "无效的用户ID");
        }

        // 验证金额与工时参数。工资表字段均由同一个接口保存，避免页面分步保存产生半成品记录。
        for (const std::string &key : {"baseSalary", "base_salary", "hourlyRate", "hourly_rate",
                                      "workHoursMonth", "work_hours_month", "paAward", "PA_Award",
                                      "pbAward", "PB_Award", "allowance", "deduction",
                                      "socialInsuranceHousingFund", "social_insurance_housing_fund"})
        {
            if (request_body.contains(key) && !request_body[key].is_null() && !request_body[key].is_number())
            {
                return ResponseHelper::validation(req, key + " 必须为数字");
            }
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            // org_scope：写路径必须与列表/详情同样按部门隔离，否则范围受限用户可按 userId
            // 改范围外员工工资。范围外查不到 → notFound（不泄露存在性），与详情接口一致。
            const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");
            mysqlx::SqlResult employeeResult = session->sql(
                                                          "SELECT u.name, COALESCE(pos.name, ''), s.id, s.pay_type, s.base_salary, "
                                                          "s.hourly_rate, s.work_hours_month, s.PA_Award, s.PB_Award, s.allowance, "
                                                          "s.deduction, s.social_insurance_housing_fund, s.total_salary, u.account_type "
                                                          "FROM users AS u "
                                                          "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                                          "LEFT JOIN salary AS s ON s.user_id = u.id "
                                                          "WHERE u.id = ? AND u.is_deleted = 0 " +
                                                          scopeFilter +
                                                          "LIMIT 1 FOR UPDATE")
                                                   .bind(goalUserId)
                                                   .execute();

            auto employeeRow = employeeResult.fetchOne();
            if (!employeeRow)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::notFound(req, "员工不存在");
            }

            const std::string employeeName = employeeRow[0].isNull() ? "" : clean_string(employeeRow[0].get<std::string>());
            const std::string employeeType = employeeRow[1].isNull() ? "" : employeeRow[1].get<std::string>();
            const std::string accountType = employeeRow[13].isNull() ? "" : employeeRow[13].get<std::string>();
            if (accountType != "staff") // 普通用户没有工资
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::validation(req, "普通用户不能创建员工工资记录");
            }

            const bool isNewSalary = employeeRow[2].isNull();
            const int salaryId = isNewSalary ? 0 : employeeRow[2].get<int>();
            const std::string DB_PayType = employeeRow[3].isNull() ? "monthly" : employeeRow[3].get<std::string>();
            const double DB_BaseSalary = employeeRow[4].isNull() ? 0.0 : employeeRow[4].get<double>();
            const double DB_HourlyRate = employeeRow[5].isNull() ? 0.0 : employeeRow[5].get<double>();
            const double DB_WorkHoursMonth = employeeRow[6].isNull() ? 0.0 : employeeRow[6].get<double>();
            const double DB_PA_Award = employeeRow[7].isNull() ? 0.0 : employeeRow[7].get<double>();
            const double DB_PB_Award = employeeRow[8].isNull() ? 0.0 : employeeRow[8].get<double>();
            const double DB_Allowance = employeeRow[9].isNull() ? 0.0 : employeeRow[9].get<double>();
            const double DB_Deduction = employeeRow[10].isNull() ? 0.0 : employeeRow[10].get<double>();
            const double DB_SocialInsurance = employeeRow[11].isNull() ? 0.0 : employeeRow[11].get<double>();
            const double DB_TotalSalary = employeeRow[12].isNull() ? 0.0 : employeeRow[12].get<double>();

            const std::string payType = request_body.value("payType", request_body.value("pay_type", DB_PayType));
            if (payType != "monthly" && payType != "hourly")
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::validation(req, "计薪方式必须为 monthly 或 hourly");
            }

            const double rawBaseSalary = getRequestDoubleWithFallback(request_body, "baseSalary", "base_salary", DB_BaseSalary);
            const double rawHourlyRate = getRequestDoubleWithFallback(request_body, "hourlyRate", "hourly_rate", DB_HourlyRate);
            const double rawWorkHoursMonth = getRequestDoubleWithFallback(request_body, "workHoursMonth", "work_hours_month", DB_WorkHoursMonth);
            const double rawPA_Award = getRequestDoubleWithFallback(request_body, "paAward", "PA_Award", DB_PA_Award);
            const double rawPB_Award = getRequestDoubleWithFallback(request_body, "pbAward", "PB_Award", DB_PB_Award);
            const double rawAllowance = getRequestDoubleWithFallback(request_body, "allowance", "allowance", DB_Allowance);
            const double rawDeduction = getRequestDoubleWithFallback(request_body, "deduction", "deduction", DB_Deduction);
            const double rawSocialInsurance = getRequestDoubleWithFallback(request_body, "socialInsuranceHousingFund", "social_insurance_housing_fund", DB_SocialInsurance);

            // 判断前端传输的工资参数是有限值
            if (!std::isfinite(rawBaseSalary) || !std::isfinite(rawHourlyRate) ||
                !std::isfinite(rawWorkHoursMonth) || !std::isfinite(rawPA_Award) ||
                !std::isfinite(rawPB_Award) || !std::isfinite(rawAllowance) ||
                !std::isfinite(rawDeduction) || !std::isfinite(rawSocialInsurance) ||
                rawBaseSalary < 0 || rawHourlyRate < 0 || rawWorkHoursMonth < 0 ||
                rawPA_Award < 0 || rawPB_Award < 0 || rawAllowance < 0 ||
                rawDeduction < 0 || rawSocialInsurance < 0)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::validation(req, "工资金额和工作时间必须为大于或等于零的有效数字");
            }

            const double baseSalary = std::round(rawBaseSalary * 100.0) / 100.0;
            const double hourlyRate = std::round(rawHourlyRate * 100.0) / 100.0;
            const double workHoursMonth = std::round(rawWorkHoursMonth * 100.0) / 100.0;
            const double PA_Award = std::round(rawPA_Award * 100.0) / 100.0;
            const double PB_Award = std::round(rawPB_Award * 100.0) / 100.0;
            const double allowance = std::round(rawAllowance * 100.0) / 100.0;
            const double deduction = std::round(rawDeduction * 100.0) / 100.0;
            const double socialInsurance = std::round(rawSocialInsurance * 100.0) / 100.0;
            const double salaryBasis = payType == "hourly" ? hourlyRate * workHoursMonth : baseSalary;
            const double totalSalary = std::round((salaryBasis + PA_Award + PB_Award + allowance - deduction - socialInsurance) * 100.0) / 100.0;

            if (!std::isfinite(totalSalary))
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::validation(req, "工资总额超出有效范围");
            }

            const bool hasChanges = payType != DB_PayType ||
                                    baseSalary != DB_BaseSalary ||
                                    hourlyRate != DB_HourlyRate ||
                                    workHoursMonth != DB_WorkHoursMonth ||
                                    PA_Award != DB_PA_Award ||
                                    PB_Award != DB_PB_Award ||
                                    allowance != DB_Allowance ||
                                    deduction != DB_Deduction ||
                                    socialInsurance != DB_SocialInsurance;

            if (!isNewSalary && !hasChanges)
            {
                session->sql("COMMIT").execute();
                return ResponseHelper::success(req, "员工工资记录已存在，并且没有需要更新的字段");
            }

            int savedSalaryId = salaryId;
            if (isNewSalary) // 添加新员工工资
            {
                mysqlx::SqlResult insertResult = session->sql(
                                                            "INSERT INTO salary (user_id, pay_type, base_salary, hourly_rate, work_hours_month, "
                                                            "PA_Award, PB_Award, allowance, deduction, social_insurance_housing_fund, total_salary) "
                                                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
                                                     .bind(goalUserId, payType, baseSalary, hourlyRate, workHoursMonth,
                                                           PA_Award, PB_Award, allowance, deduction, socialInsurance, totalSalary)
                                                     .execute();

                if (insertResult.getAffectedItemsCount() != 1)
                {
                    rollbackTransactionQuietly(*session);
                    return ResponseHelper::error(req, "给新员工添加工资失败");
                }
                savedSalaryId = static_cast<int>(insertResult.getAutoIncrementValue());
            }
            else // 更新员工工资
            {
                const std::string changeReason = getJsonString(request_body, "changeReason");
                if (changeReason.empty())
                {
                    rollbackTransactionQuietly(*session);
                    return ResponseHelper::validation(req, "修改已有工资记录必须填写修改说明");
                }
                const int changedBy = currentRequestUserId(req).value_or(0);
                if (changedBy <= 0)
                {
                    rollbackTransactionQuietly(*session);
                    return ResponseHelper::validation(req, "无法识别工资修改人");
                }

                mysqlx::SqlResult updateResult = session->sql(
                                                            "UPDATE salary SET pay_type = ?, base_salary = ?, hourly_rate = ?, work_hours_month = ?, "
                                                            "PA_Award = ?, PB_Award = ?, allowance = ?, deduction = ?, "
                                                            "social_insurance_housing_fund = ?, total_salary = ?, is_manually_modified = 1, "
                                                            "last_modified_by = ?, last_modified_at = NOW() "
                                                            "WHERE id = ? AND user_id = ?")
                                                     .bind(payType, baseSalary, hourlyRate, workHoursMonth, PA_Award, PB_Award,
                                                           allowance, deduction, socialInsurance, totalSalary, changedBy, salaryId, goalUserId)
                                                     .execute();

                if (updateResult.getAffectedItemsCount() != 1)
                {
                    rollbackTransactionQuietly(*session);
                    return ResponseHelper::error(req, "更新员工工资失败");
                }

                const nlohmann::json beforeSnapshot = {
                    {"pay_type", DB_PayType}, {"base_salary", DB_BaseSalary},
                    {"hourly_rate", DB_HourlyRate}, {"work_hours_month", DB_WorkHoursMonth},
                    {"pa_award", DB_PA_Award}, {"pb_award", DB_PB_Award},
                    {"allowance", DB_Allowance}, {"deduction", DB_Deduction},
                    {"social_insurance_housing_fund", DB_SocialInsurance}, {"total_salary", DB_TotalSalary}};
                const nlohmann::json afterSnapshot = {
                    {"pay_type", payType}, {"base_salary", baseSalary},
                    {"hourly_rate", hourlyRate}, {"work_hours_month", workHoursMonth},
                    {"pa_award", PA_Award}, {"pb_award", PB_Award},
                    {"allowance", allowance}, {"deduction", deduction},
                    {"social_insurance_housing_fund", socialInsurance}, {"total_salary", totalSalary}};
                session->sql("INSERT INTO salaryChangeRecord "
                             "(salary_id, user_id, changed_by, change_reason, before_snapshot, after_snapshot) "
                             "VALUES (?, ?, ?, ?, CAST(? AS JSON), CAST(? AS JSON))")
                    .bind(salaryId, goalUserId, changedBy, changeReason, beforeSnapshot.dump(), afterSnapshot.dump())
                    .execute();
            }

            nlohmann::json data = {
                {"salary_id", savedSalaryId},
                {"employee_name", employeeName},
                {"type", employeeType},
                {"total_salary", totalSalary}};

            if (isNewSalary)
            {
                mysqlx::SqlResult countResult = session->sql(
                                                           "SELECT COUNT(*) "
                                                           "FROM salary AS s "
                                                           "JOIN users AS u ON u.id = s.user_id "
                                                           "WHERE u.is_deleted = 0")
                                                    .execute();
                auto countRow = countResult.fetchOne();
                data["total_count"] = countRow && countRow[0].isNull() ? 0 : countRow[0].get<int>();
            }

            session->sql("COMMIT").execute();
            FinanceHomeDataBroadcaster::instance().notifyHomeDataChanged();
            AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
            return isNewSalary ? ResponseHelper::created(req, data) : ResponseHelper::success(req, data);
        }
        catch (const std::invalid_argument &e)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, e.what());
        }
        catch (...)
        {
            rollbackTransactionQuietly(*session);
            throw;
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "财务", "添加或修改员工工资", "Failed to update employee salary: " + std::string(e.what()));
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response financeHandler::getSalarySummary(const crow::request &req, int page)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        if (page < 1)
        {
            return ResponseHelper::error(req, "无效的页码");
        }

        const int pageSize = 150;
        const int offset = (page - 1) * pageSize; // 计算偏移量
        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT s.id, u.name, COALESCE(pos.name, ''), COALESCE(s.total_salary, 0) "
                                             "FROM salary AS s "
                                             "JOIN users AS u ON u.id = s.user_id "
                                             "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                             "WHERE u.is_deleted = 0 " +
                                             scopeFilter +
                                             "ORDER BY COALESCE(s.total_salary, 0) DESC, u.id ASC "
                                             "LIMIT ?, ?")
                                       .bind(offset, pageSize)
                                       .execute();

        mysqlx::SqlResult countResult = dbManager->getSession()
                                            ->sql("SELECT COUNT(*) "
                                                  "FROM salary AS s "
                                                  "JOIN users AS u ON u.id = s.user_id "
                                                  "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                                  "WHERE u.is_deleted = 0 " +
                                                  scopeFilter)
                                            .execute();

        nlohmann::json list = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json employeeSalary;
            employeeSalary["salary_id"] = row[0].isNull() ? 0 : row[0].get<int>();
            employeeSalary["employee_name"] = row[1].isNull() ? "" : clean_string(row[1].get<std::string>());
            employeeSalary["type"] = row[2].isNull() ? "" : row[2].get<std::string>();
            employeeSalary["total_salary"] = row[3].isNull() ? 0.0 : row[3].get<double>();

            list.push_back(employeeSalary);
        }

        nlohmann::json data = {
            {"list", list},
            {"total_count", countResult.fetchOne()[0].get<int>()},
            {"page", page},
            {"page_size", pageSize}};

        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "财务", "获取员工工资列表摘要", "Failed to get salary summary: " + std::string(e.what()));
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response financeHandler::getSalaryInformation(const crow::request &req, int salaryId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        if (salaryId <= 0)
        {
            return ResponseHelper::error(req, "无效的工资单编号");
        }

        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");
        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT u.id, s.id, u.name, COALESCE(pos.name, ''), s.pay_type, s.base_salary, "
                                             "s.hourly_rate, s.work_hours_month, s.PA_Award, s.PB_Award, s.allowance, "
                                             "s.deduction, s.social_insurance_housing_fund, s.total_salary, "
                                             "s.is_manually_modified, CAST(s.last_modified_at AS CHAR), CAST(s.updated_at AS CHAR) "
                                             "FROM salary AS s "
                                             "JOIN users AS u ON u.id = s.user_id "
                                             "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                             "WHERE s.id = ? AND u.is_deleted = 0 " +
                                             scopeFilter)
                                       .bind(salaryId)
                                       .execute();

        auto row = result.fetchOne();
        if (!row)
        {
            return ResponseHelper::notFound(req, "工资单未找到");
        }

        nlohmann::json data;
        data["user_id"] = row[0].isNull() ? 0 : row[0].get<int>();
        data["salary_id"] = row[1].isNull() ? 0 : row[1].get<int>();
        data["name"] = row[2].isNull() ? "" : clean_string(row[2].get<std::string>());
        data["type"] = row[3].isNull() ? "" : row[3].get<std::string>();
        data["pay_type"] = row[4].isNull() ? "monthly" : row[4].get<std::string>();
        data["base_salary"] = row[5].isNull() ? 0.0 : row[5].get<double>();
        data["hourly_rate"] = row[6].isNull() ? 0.0 : row[6].get<double>();
        data["work_hours_month"] = row[7].isNull() ? 0.0 : row[7].get<double>();
        data["PA_Award"] = row[8].isNull() ? 0.0 : row[8].get<double>();
        data["PB_Award"] = row[9].isNull() ? 0.0 : row[9].get<double>();
        data["allowance"] = row[10].isNull() ? 0.0 : row[10].get<double>();
        data["deduction"] = row[11].isNull() ? 0.0 : row[11].get<double>();
        data["social_insurance_housing_fund"] = row[12].isNull() ? 0.0 : row[12].get<double>();
        data["total_salary"] = row[13].isNull() ? 0.0 : row[13].get<double>();
        data["is_manually_modified"] = !row[14].isNull() && row[14].get<int>() != 0;
        data["last_modified_at"] = row[15].isNull() ? "" : row[15].get<std::string>();
        data["updated_at"] = row[16].isNull() ? "" : row[16].get<std::string>();

        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "财务", "获取员工工资详情", "Failed to get salary information: " + std::string(e.what()));
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response financeHandler::searchSalaryEmployees(const crow::request &req, const nlohmann::json &requestBody)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        const std::string keyword = getJsonString(requestBody, "keyword");
        const std::string likeKeyword = "%" + keyword + "%";
        const int page = normalizePage(getJsonInt(requestBody, "page", 1));
        const int pageSize = normalizePageSize(getJsonInt(requestBody, "pageSize", 10), 10, 100);
        const int offset = (page - 1) * pageSize;
        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT u.id, COALESCE(u.position_id, 0), COALESCE(pos.name, ''), u.name, p.phone, u.email, "
                                             "COALESCE(s.pay_type, 'monthly'), COALESCE(s.base_salary, 0), COALESCE(s.hourly_rate, 0), "
                                             "COALESCE(s.work_hours_month, 0), COALESCE(s.PA_Award, 0), COALESCE(s.PB_Award, 0), "
                                             "COALESCE(s.allowance, 0), COALESCE(s.deduction, 0), "
                                             "COALESCE(s.social_insurance_housing_fund, 0), COALESCE(s.total_salary, 0), "
                                             "COALESCE(s.is_manually_modified, 0), CAST(s.last_modified_at AS CHAR), CAST(s.updated_at AS CHAR) "
                                             "FROM users AS u "
                                             "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                             "LEFT JOIN phones AS p ON p.user_id = u.id "
                                             "LEFT JOIN salary AS s ON s.user_id = u.id "
                                             "WHERE u.is_deleted = 0 AND u.account_type = 'staff' "
                                             "AND (? = '' OR COALESCE(u.name, '') LIKE ? OR COALESCE(pos.name, '') LIKE ? "
                                             "OR COALESCE(p.phone, '') LIKE ? OR COALESCE(u.email, '') LIKE ?) " +
                                             scopeFilter +
                                             "ORDER BY u.id ASC "
                                             "LIMIT ?, ?")
                                       .bind(keyword, likeKeyword, likeKeyword, likeKeyword, likeKeyword, offset, pageSize)
                                       .execute();

        mysqlx::SqlResult countResult = dbManager->getSession()
                                            ->sql("SELECT COUNT(DISTINCT u.id) "
                                                  "FROM users AS u "
                                                  "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                                  "LEFT JOIN phones AS p ON p.user_id = u.id "
                                                  "WHERE u.is_deleted = 0 AND u.account_type = 'staff' "
                                                  "AND (? = '' OR COALESCE(u.name, '') LIKE ? OR COALESCE(pos.name, '') LIKE ? "
                                                  "OR COALESCE(p.phone, '') LIKE ? OR COALESCE(u.email, '') LIKE ?) " +
                                                  scopeFilter)
                                            .bind(keyword, likeKeyword, likeKeyword, likeKeyword, likeKeyword)
                                            .execute();

        mysqlx::SqlResult summaryResult = dbManager->getSession()
                                              ->sql("SELECT COUNT(*), COALESCE(SUM(COALESCE(s.total_salary, 0)), 0) "
                                                    "FROM users AS u "
                                                    "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                                    "LEFT JOIN salary AS s ON s.user_id = u.id "
                                                    "WHERE u.is_deleted = 0 AND u.account_type = 'staff' " +
                                                    scopeFilter)
                                              .execute();

        nlohmann::json employees = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json employee;
            employee["id"] = row[0].isNull() ? 0 : row[0].get<int>();
            employee["type_id"] = row[1].isNull() ? nullptr : nlohmann::json(row[1].get<int>());
            employee["type_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
            employee["name"] = row[3].isNull() ? "" : clean_string(row[3].get<std::string>());
            employee["phone"] = row[4].isNull() ? "" : clean_string(row[4].get<std::string>());
            employee["email"] = row[5].isNull() ? "" : clean_string(row[5].get<std::string>());
            employee["pay_type"] = row[6].isNull() ? "monthly" : row[6].get<std::string>();
            employee["base_salary"] = row[7].isNull() ? 0.0 : row[7].get<double>();
            employee["hourly_rate"] = row[8].isNull() ? 0.0 : row[8].get<double>();
            employee["work_hours_month"] = row[9].isNull() ? 0.0 : row[9].get<double>();
            employee["pa_award"] = row[10].isNull() ? 0.0 : row[10].get<double>();
            employee["pb_award"] = row[11].isNull() ? 0.0 : row[11].get<double>();
            employee["allowance"] = row[12].isNull() ? 0.0 : row[12].get<double>();
            employee["deduction"] = row[13].isNull() ? 0.0 : row[13].get<double>();
            employee["social_insurance_housing_fund"] = row[14].isNull() ? 0.0 : row[14].get<double>();
            employee["total_salary"] = row[15].isNull() ? 0.0 : row[15].get<double>();
            employee["is_manually_modified"] = !row[16].isNull() && row[16].get<int>() != 0;
            employee["last_modified_at"] = row[17].isNull() ? "" : row[17].get<std::string>();
            employee["updated_at"] = row[18].isNull() ? "" : row[18].get<std::string>();
            employees.push_back(employee);
        }

        auto summaryRow = summaryResult.fetchOne();
        const int employeeCount = summaryRow && !summaryRow[0].isNull() ? summaryRow[0].get<int>() : 0;
        const double monthlyPayroll = summaryRow && !summaryRow[1].isNull() ? summaryRow[1].get<double>() : 0.0;
        const nlohmann::json homeData = buildHomeData(req);

        nlohmann::json data = {
            {"employees", employees},
            {"total", countResult.fetchOne()[0].get<int>()},
            {"page", page},
            {"pageSize", pageSize},
            {"summary", {
                            {"employeeCount", employeeCount},
                            {"monthlyPayroll", monthlyPayroll},
                            {"todayCost", homeData.value("dailyCost", 0.0)},
                            {"todayProfit", homeData.value("dailyProfit", 0.0)},
                        }}};

        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response financeHandler::getExpenseData(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        auto session = dbManager->getSession();

        // 获取最近10条日常记录
        mysqlx::SqlResult dailyRecordsResult = session->sql("SELECT id, salesCount, costCount, profitCount, "
                                                            "CAST(created_at AS CHAR) "
                                                            "FROM monthlyFinancialRecord "
                                                            "ORDER BY created_at DESC "
                                                            "LIMIT 10")
                                                   .execute();

        // 获取最近10条月度记录
        mysqlx::SqlResult monthlyRecordsResult = session->sql("SELECT id, salesCount, costCount, profitCount, "
                                                              "CAST(created_at AS CHAR) "
                                                              "FROM financialRecord "
                                                              "WHERE record_type = 'month' "
                                                              "ORDER BY created_at DESC "
                                                              "LIMIT 10")
                                                     .execute();

        nlohmann::json response;
        response["dailyRecords"] = nlohmann::json::array();
        response["monthlyRecords"] = nlohmann::json::array();

        double totalPayroll = 0.0;
        int employeeCount = 0;

        for (auto row : dailyRecordsResult)
        {
            nlohmann::json record;
            record["id"] = row[0].isNull() ? 0 : row[0].get<int>();
            record["salesCount"] = row[1].isNull() ? 0.0 : row[1].get<double>();
            record["costCount"] = row[2].isNull() ? 0.0 : row[2].get<double>();
            record["profitCount"] = row[3].isNull() ? 0.0 : row[3].get<double>();
            record["created_at"] = row[4].isNull() ? "" : row[4].get<std::string>();
            response["dailyRecords"].push_back(record);
        }

        for (auto row : monthlyRecordsResult)
        {
            nlohmann::json record;
            record["id"] = row[0].isNull() ? 0 : row[0].get<int>();
            record["salesCount"] = row[1].isNull() ? 0.0 : row[1].get<double>();
            record["costCount"] = row[2].isNull() ? 0.0 : row[2].get<double>();
            record["profitCount"] = row[3].isNull() ? 0.0 : row[3].get<double>();
            record["created_at"] = row[4].isNull() ? "" : row[4].get<std::string>();
            response["monthlyRecords"].push_back(record);
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response financeHandler::getPayrollEmployees(const crow::request &req, const nlohmann::json &requestBody)
{
    try
    {
        auto session = dbManager->getSession();
        const int periodId = ensureCurrentPayrollPeriod(*session);
        const std::string keyword = getJsonString(requestBody, "keyword");
        const std::string likeKeyword = "%" + keyword + "%";
        const int page = normalizePage(getJsonInt(requestBody, "page", 1));
        const int pageSize = normalizePageSize(getJsonInt(requestBody, "pageSize", 100), 10, 200);
        const int offset = (page - 1) * pageSize;
        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");
        const std::string baseFrom =
            " FROM users AS u LEFT JOIN positions AS pos ON pos.id=u.position_id "
            "LEFT JOIN phones AS p ON p.user_id=u.id "
            "LEFT JOIN salaryProfile AS sp ON sp.user_id=u.id AND sp.effective_to IS NULL "
            "LEFT JOIN salary AS s ON s.user_id=u.id AND s.payroll_period_id=? "
            "WHERE u.is_deleted=0 AND u.account_type='staff' "
            "AND (?='' OR u.name LIKE ? OR pos.name LIKE ? OR p.phone LIKE ? OR u.email LIKE ?) " + scopeFilter;
        const std::string selectSql =
            "SELECT u.id, COALESCE(pos.name,''), u.name, p.phone, u.email, "
            "COALESCE(s.id,0), sp.id, COALESCE(s.pay_type,sp.pay_type,'monthly'), "
            "COALESCE(s.base_salary,sp.base_salary,0), COALESCE(s.hourly_rate,sp.hourly_rate,0), "
            "COALESCE(s.work_hours_month,0), COALESCE(s.attendance_award,0), COALESCE(s.performance_award,0), "
            "COALESCE(s.allowance,0), COALESCE(s.deduction,0), COALESCE(s.social_insurance_housing_fund,COALESCE(sp.social_insurance_housing_fund,0)), "
            "COALESCE(s.total_salary,0), COALESCE(s.review_status,'pending'), COALESCE(s.is_manually_modified,0), "
            "(SELECT COUNT(*) FROM salaryChangeRecord cr WHERE cr.salary_id=s.id) " + baseFrom +
            " ORDER BY u.id ASC LIMIT ?,?";
        auto result = session->sql(selectSql)
                          .bind(periodId, keyword, likeKeyword, likeKeyword, likeKeyword, likeKeyword, offset, pageSize)
                          .execute();
        auto countResult = session->sql("SELECT COUNT(DISTINCT u.id) " + baseFrom)
                               .bind(periodId, keyword, likeKeyword, likeKeyword, likeKeyword, likeKeyword)
                               .execute();
        nlohmann::json employees = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json item = {
                {"id", row[0].get<int>()}, {"type_name", row[1].get<std::string>()}, {"name", clean_string(row[2].get<std::string>())},
                {"phone", row[3].isNull() ? "" : row[3].get<std::string>()}, {"email", row[4].isNull() ? "" : row[4].get<std::string>()},
                {"salary_id", row[5].get<long long>()}, {"salary_profile_id", row[6].isNull() ? 0 : row[6].get<int>()},
                {"pay_type", row[7].get<std::string>()}, {"base_salary", row[8].isNull() ? 0.0 : row[8].get<double>()},
                {"hourly_rate", row[9].isNull() ? 0.0 : row[9].get<double>()}, {"work_hours_month", row[10].get<double>()},
                {"attendance_award", row[11].get<double>()}, {"performance_award", row[12].get<double>()}, {"allowance", row[13].get<double>()},
                {"deduction", row[14].get<double>()}, {"social_insurance_housing_fund", row[15].get<double>()}, {"total_salary", row[16].get<double>()},
                {"review_status", row[17].get<std::string>()}, {"is_manually_modified", row[18].get<int>() != 0}, {"change_count", row[19].get<int>()}};
            employees.push_back(item);
        }
        const auto countRow = countResult.fetchOne();
        const int total = countRow && !countRow[0].isNull() ? countRow[0].get<int>() : 0;
        const auto period = session->sql("SELECT status,version_no,total_salary FROM payrollPeriod WHERE id=?").bind(periodId).execute().fetchOne();
        return ResponseHelper::success(req, { {"employees", employees}, {"total", total}, {"page", page}, {"pageSize", pageSize},
            {"period", {{"id", periodId}, {"status", period ? period[0].get<std::string>() : "first_review"}, {"versionNo", period ? period[1].get<int>() : 1}, {"totalSalary", period ? period[2].get<double>() : 0.0}}} });
    }
    catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}

crow::response financeHandler::savePayrollEmployee(const crow::request &req, int goalUserId)
{
    try
    {
        crow::response res;
        auto body = validateRequest(req, res);
        if (!body) return res;
        if (goalUserId <= 0) return ResponseHelper::error(req, "无效的用户ID");
        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();
        const int periodId = ensureCurrentPayrollPeriod(*session);
        const std::string payType = body->value("pay_type", body->value("payType", "monthly"));
        const double base = getRequestDoubleWithFallback(*body, "base_salary", "baseSalary", 0.0);
        const double hourly = getRequestDoubleWithFallback(*body, "hourly_rate", "hourlyRate", 0.0);
        const double social = getRequestDoubleWithFallback(*body, "social_insurance_housing_fund", "socialInsuranceHousingFund", 0.0);
        const double hours = getRequestDoubleWithFallback(*body, "work_hours_month", "workHoursMonth", 0.0);
        const double attendance = getRequestDoubleWithFallback(*body, "attendance_award", "attendanceAward", 0.0);
        const double performance = getRequestDoubleWithFallback(*body, "performance_award", "performanceAward", 0.0);
        const double allowance = getRequestDoubleWithFallback(*body, "allowance", "allowance", 0.0);
        const double deduction = getRequestDoubleWithFallback(*body, "deduction", "deduction", 0.0);
        if ((payType != "monthly" && payType != "hourly") || base < 0 || hourly < 0 || social < 0 || hours < 0 || attendance < 0 || performance < 0 || allowance < 0 || deduction < 0)
        { rollbackTransactionQuietly(*session); return ResponseHelper::validation(req, "工资字段值无效"); }
        const std::string effectiveFrom = body->value("effective_from", body->value("effectiveFrom", ""));
        const std::string date = effectiveFrom.empty() ? "DATE_FORMAT(CURDATE(), '%Y-%m-01')" : "?";
        const std::string profileSql = "INSERT INTO salaryProfile (user_id,pay_type,base_salary,hourly_rate,social_insurance_housing_fund,effective_from) VALUES (?,?,?,?,?," + date + ") ON DUPLICATE KEY UPDATE pay_type=VALUES(pay_type),base_salary=VALUES(base_salary),hourly_rate=VALUES(hourly_rate),social_insurance_housing_fund=VALUES(social_insurance_housing_fund),effective_to=NULL";
        auto profileStmt = session->sql(profileSql)
            .bind(goalUserId, payType,
                  payType == "monthly" ? mysqlx::Value(base) : mysqlx::Value(),
                  payType == "hourly" ? mysqlx::Value(hourly) : mysqlx::Value(), social);
        if (date == "?") profileStmt.bind(effectiveFrom);
        profileStmt.execute();
        auto profile = session->sql("SELECT id FROM salaryProfile WHERE user_id=? AND effective_to IS NULL ORDER BY effective_from DESC LIMIT 1").bind(goalUserId).execute().fetchOne();
        const int profileId = profile ? profile[0].get<int>() : 0;
        const double total = std::round(((payType == "hourly" ? hourly * hours : base) + attendance + performance + allowance - deduction - social) * 100.0) / 100.0;
        const bool manual = body->contains("change_reason") && !body->value("change_reason", "").empty();
        auto upsert = session->sql("INSERT INTO salary (payroll_period_id,salary_profile_id,user_id,pay_type,base_salary,hourly_rate,work_hours_month,attendance_award,performance_award,allowance,deduction,social_insurance_housing_fund,total_salary,review_status,is_manually_modified) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,'pending',?) ON DUPLICATE KEY UPDATE salary_profile_id=VALUES(salary_profile_id),pay_type=VALUES(pay_type),base_salary=VALUES(base_salary),hourly_rate=VALUES(hourly_rate),work_hours_month=VALUES(work_hours_month),attendance_award=VALUES(attendance_award),performance_award=VALUES(performance_award),allowance=VALUES(allowance),deduction=VALUES(deduction),social_insurance_housing_fund=VALUES(social_insurance_housing_fund),total_salary=VALUES(total_salary),is_manually_modified=IF(?=1,1,is_manually_modified),review_status=IF(review_status='locked',review_status,'pending')")
            .bind(periodId, profileId, goalUserId, payType,
                  payType == "monthly" ? mysqlx::Value(base) : mysqlx::Value(),
                  payType == "hourly" ? mysqlx::Value(hourly) : mysqlx::Value(),
                  hours, attendance, performance, allowance, deduction, social, total, manual ? 1 : 0, manual ? 1 : 0);
        upsert.execute();
        if (manual)
        {
            auto salary = session->sql("SELECT id FROM salary WHERE payroll_period_id=? AND user_id=?").bind(periodId, goalUserId).execute().fetchOne();
            session->sql("INSERT INTO salaryChangeRecord (salary_id,changed_field,before_value,after_value,changed_by,change_reason,evidence_path) VALUES (?,?,NULL,?, ?,?,?)")
                .bind(salary[0].get<long long>(), "payroll_snapshot", "updated", currentRequestUserId(req).value_or(0), body->value("change_reason", ""), body->value("evidence_path", "")).execute();
        }
        session->sql("COMMIT").execute();
        FinanceHomeDataBroadcaster::instance().notifyHomeDataChanged();
        return ResponseHelper::success(req, "工资快照已保存");
    }
    catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}

crow::response financeHandler::submitPayrollReview(const crow::request &req)
{
    try { auto session = dbManager->getSession(); const int periodId = ensureCurrentPayrollPeriod(*session); auto missing = session->sql("SELECT COUNT(*) FROM users u LEFT JOIN salaryProfile sp ON sp.user_id=u.id AND sp.effective_to IS NULL WHERE u.account_type='staff' AND u.is_deleted=0 AND sp.id IS NULL").execute().fetchOne(); if (missing && missing[0].get<int>() > 0) return ResponseHelper::validation(req, "仍有员工缺少生效薪资配置"); session->sql("UPDATE payrollPeriod SET status='second_review',reviewed_by=?,reviewed_at=NOW() WHERE id=? AND status='first_review'").bind(currentRequestUserId(req).value_or(0), periodId).execute(); return ResponseHelper::success(req, "已提交主管复审"); } catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}

crow::response financeHandler::lockPayroll(const crow::request &req)
{
    try { auto session = dbManager->getSession(); const int periodId = ensureCurrentPayrollPeriod(*session); auto result = session->sql("UPDATE payrollPeriod SET status='locked',locked_by=?,locked_at=NOW() WHERE id=? AND status='second_review'").bind(currentRequestUserId(req).value_or(0), periodId).execute(); if (result.getAffectedItemsCount() != 1) return ResponseHelper::validation(req, "当前工资周期尚未完成复审"); session->sql("UPDATE salary SET review_status='locked' WHERE payroll_period_id=?").bind(periodId).execute(); return ResponseHelper::success(req, "工资周期已锁定"); } catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}
