#include "financeHandler.h"
#include "../../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../../services/realtime/financeBroadcaster/financeHomeDataBroadcaster.h"
#include "../../../services/rbac/RbacService.h"
#include "../../../utils/permissions/Permissions.h"
#include "../../../utils/requestUtils/RequestUtils.h"
#include <cmath>
#include <iomanip>
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

    bool hasUnrestrictedOrgScope(
        const crow::request &req,
        const std::shared_ptr<DatabaseManagerInterface> &dbManager)
    {
        const std::optional<int> userId = currentRequestUserId(req);
        return userId.has_value() &&
               RbacService::loadEffectiveOrgScope(dbManager, userId.value()).unrestricted;
    }

    std::string moneyValue(double value)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << value;
        return stream.str();
    }

    int ensureCurrentPayrollPeriod(mysqlx::Session &session)
    {
        try
        {
            session.sql("INSERT INTO payrollPeriod (payroll_month, status, version_no) "
                        "SELECT DATE_FORMAT(CURDATE(), '%Y-%m-01'), 'first_review', 1 "
                        "FROM DUAL WHERE NOT EXISTS ("
                        "SELECT 1 FROM payrollPeriod WHERE payroll_month = DATE_FORMAT(CURDATE(), '%Y-%m-01') "
                        "AND status IN ('calculating','first_review','second_review'))")
                .execute();
        }
        catch (const mysqlx::Error &e)
        {
            // 两个首个请求同时初始化月份时，唯一键冲突只表示另一请求已经完成初始化。
            if (std::string(e.what()).find("Duplicate") == std::string::npos)
            {
                throw;
            }
        }
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

    void refreshPayrollPeriodTotal(mysqlx::Session &session, int periodId)
    {
        session.sql("UPDATE payrollPeriod p SET total_salary = ("
                    "SELECT COALESCE(SUM(s.total_salary), 0) FROM salary s "
                    "WHERE s.payroll_period_id = p.id) WHERE p.id = ?")
            .bind(periodId)
            .execute();
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
                                             "s.hourly_rate, s.work_hours_month, s.attendance_award, s.performance_award, s.allowance, "
                                             "s.deduction, s.social_insurance_housing_fund, s.total_salary, "
                                             "s.is_manually_modified, CAST(s.updated_at AS CHAR) "
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
        data["last_modified_at"] = "";
        data["updated_at"] = row[15].isNull() ? "" : row[15].get<std::string>();

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
                                             "COALESCE(s.work_hours_month, 0), COALESCE(s.attendance_award, 0), COALESCE(s.performance_award, 0), "
                                             "COALESCE(s.allowance, 0), COALESCE(s.deduction, 0), "
                                             "COALESCE(s.social_insurance_housing_fund, 0), COALESCE(s.total_salary, 0), "
                                             "COALESCE(s.is_manually_modified, 0), CAST(s.updated_at AS CHAR) "
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
            employee["last_modified_at"] = "";
            employee["updated_at"] = row[17].isNull() ? "" : row[17].get<std::string>();
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
        const std::string payType = getJsonString(requestBody, "payType").empty() ? "all" : getJsonString(requestBody, "payType");
        const std::string modified = getJsonString(requestBody, "modified").empty() ? "all" : getJsonString(requestBody, "modified");
        const std::string reviewStatus = getJsonString(requestBody, "reviewStatus").empty() ? "all" : getJsonString(requestBody, "reviewStatus");
        const int page = normalizePage(getJsonInt(requestBody, "page", 1));
        const int pageSize = normalizePageSize(getJsonInt(requestBody, "pageSize", 100), 10, 200);
        const int offset = (page - 1) * pageSize;
        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");
        const std::string baseFrom =
            " FROM users AS u LEFT JOIN positions AS pos ON pos.id=u.position_id "
            "LEFT JOIN phones AS p ON p.user_id=u.id "
            "LEFT JOIN salaryProfile AS sp ON sp.id=(SELECT sp2.id FROM salaryProfile sp2 "
            "WHERE sp2.user_id=u.id AND sp2.effective_from<=DATE_FORMAT(CURDATE(), '%Y-%m-01') "
            "AND (sp2.effective_to IS NULL OR sp2.effective_to>=DATE_FORMAT(CURDATE(), '%Y-%m-01')) "
            "ORDER BY sp2.effective_from DESC, sp2.id DESC LIMIT 1) "
            "LEFT JOIN salary AS s ON s.user_id=u.id AND s.payroll_period_id=? "
            "WHERE u.is_deleted=0 AND u.account_type='staff' "
            "AND (?='' OR u.name LIKE ? OR pos.name LIKE ? OR p.phone LIKE ? OR u.email LIKE ?) "
            "AND (?='all' OR COALESCE(s.pay_type,sp.pay_type,'monthly')=?) "
            "AND (?='all' OR (?='modified' AND COALESCE(s.is_manually_modified,0)=1) "
            "OR (?='clean' AND COALESCE(s.is_manually_modified,0)=0)) "
            "AND (?='all' OR COALESCE(s.review_status,'pending')=?) " + scopeFilter;
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
                          .bind(periodId, keyword, likeKeyword, likeKeyword, likeKeyword, likeKeyword,
                                payType, payType, modified, modified, modified, reviewStatus, reviewStatus,
                                offset, pageSize)
                          .execute();
        auto countResult = session->sql("SELECT COUNT(DISTINCT u.id) " + baseFrom)
                               .bind(periodId, keyword, likeKeyword, likeKeyword, likeKeyword, likeKeyword,
                                     payType, payType, modified, modified, modified, reviewStatus, reviewStatus)
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
        const std::string periodFrom =
            " FROM users AS u LEFT JOIN positions AS pos ON pos.id=u.position_id "
            "LEFT JOIN salaryProfile AS sp ON sp.id=(SELECT sp2.id FROM salaryProfile sp2 "
            "WHERE sp2.user_id=u.id AND sp2.effective_from<=DATE_FORMAT(CURDATE(), '%Y-%m-01') "
            "AND (sp2.effective_to IS NULL OR sp2.effective_to>=DATE_FORMAT(CURDATE(), '%Y-%m-01')) "
            "ORDER BY sp2.effective_from DESC, sp2.id DESC LIMIT 1) "
            "LEFT JOIN salary AS s ON s.user_id=u.id AND s.payroll_period_id=? "
            "WHERE u.is_deleted=0 AND u.account_type='staff' " + scopeFilter;
        const auto periodStats = session->sql(
            "SELECT COUNT(DISTINCT u.id), "
            "SUM(CASE WHEN sp.id IS NULL THEN 1 ELSE 0 END), "
            "SUM(CASE WHEN s.id IS NULL OR s.review_status='pending' THEN 1 ELSE 0 END), "
            "SUM(CASE WHEN COALESCE(s.is_manually_modified,0)=1 THEN 1 ELSE 0 END), "
            "SUM(CASE WHEN s.review_status IN ('first_reviewed','second_reviewed','locked') THEN 1 ELSE 0 END), "
            "COALESCE(SUM(COALESCE(s.total_salary,0)),0)" + periodFrom)
            .bind(periodId)
            .execute()
            .fetchOne();
        const auto period = session->sql("SELECT status,version_no,total_salary,review_note FROM payrollPeriod WHERE id=?").bind(periodId).execute().fetchOne();
        const int periodEmployeeCount = periodStats && !periodStats[0].isNull() ? periodStats[0].get<int>() : 0;
        const int unconfiguredCount = periodStats && !periodStats[1].isNull() ? periodStats[1].get<int>() : 0;
        const int pendingReviewCount = periodStats && !periodStats[2].isNull() ? periodStats[2].get<int>() : 0;
        const int modifiedCount = periodStats && !periodStats[3].isNull() ? periodStats[3].get<int>() : 0;
        const int reviewedCount = periodStats && !periodStats[4].isNull() ? periodStats[4].get<int>() : 0;
        const double computedTotal = periodStats && !periodStats[5].isNull() ? periodStats[5].get<double>() : 0.0;
        return ResponseHelper::success(req, { {"employees", employees}, {"total", total}, {"page", page}, {"pageSize", pageSize},
            {"period", {{"id", periodId}, {"status", period ? period[0].get<std::string>() : "first_review"},
                         {"versionNo", period ? period[1].get<int>() : 1},
                         {"totalSalary", period ? period[2].get<double>() : computedTotal},
                         {"reviewNote", period && !period[3].isNull() ? period[3].get<std::string>() : ""},
                         {"employeeCount", periodEmployeeCount}, {"unconfiguredCount", unconfiguredCount},
                         {"pendingReviewCount", pendingReviewCount}, {"modifiedCount", modifiedCount},
                         {"reviewedCount", reviewedCount}}} });
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
        const int operatorId = currentRequestUserId(req).value_or(0);
        if (operatorId <= 0) return ResponseHelper::permission_denied(req, "无法识别工资修改人");
        const std::string changeReason = body->value("change_reason", body->value("changeReason", std::string{}));
        if (changeReason.empty()) return ResponseHelper::validation(req, "修改工资快照必须填写原因");
        if (changeReason.size() > 500) return ResponseHelper::validation(req, "修改原因不能超过500个字符");

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();
        const int periodId = ensureCurrentPayrollPeriod(*session);
        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");
        const auto scopedEmployee = session->sql(
            "SELECT u.id FROM users u LEFT JOIN positions pos ON pos.id=u.position_id "
            "WHERE u.id=? AND u.is_deleted=0 AND u.account_type='staff' " + scopeFilter + " LIMIT 1 FOR UPDATE")
            .bind(goalUserId)
            .execute()
            .fetchOne();
        if (!scopedEmployee)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::notFound(req, "员工不存在");
        }
        const auto periodState = session->sql("SELECT status FROM payrollPeriod WHERE id=? FOR UPDATE").bind(periodId).execute().fetchOne();
        if (!periodState || periodState[0].isNull() || periodState[0].get<std::string>() != "first_review")
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "仅财务初审阶段允许修改工资快照");
        }

        const auto existing = session->sql(
            "SELECT id,salary_profile_id,pay_type,base_salary,hourly_rate,work_hours_month,attendance_award,"
            "performance_award,allowance,deduction,social_insurance_housing_fund,total_salary "
            "FROM salary WHERE payroll_period_id=? AND user_id=? FOR UPDATE")
            .bind(periodId, goalUserId).execute().fetchOne();
        const auto profile = session->sql(
            "SELECT sp.id,sp.pay_type,sp.base_salary,sp.hourly_rate,sp.social_insurance_housing_fund "
            "FROM salaryProfile sp JOIN payrollPeriod p ON p.id=? WHERE sp.user_id=? "
            "AND sp.effective_from<=p.payroll_month "
            "AND (sp.effective_to IS NULL OR sp.effective_to>=p.payroll_month) "
            "ORDER BY sp.effective_from DESC,sp.id DESC LIMIT 1")
            .bind(periodId, goalUserId).execute().fetchOne();
        if (!existing && !profile)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "请先配置当前周期生效的薪资标准");
        }

        const int profileId = existing && !existing[1].isNull()
                                  ? existing[1].get<int>()
                                  : (profile ? profile[0].get<int>() : 0);
        const std::string payType = existing ? existing[2].get<std::string>() : profile[1].get<std::string>();
        const double base = existing && !existing[3].isNull() ? existing[3].get<double>()
                            : (profile && !profile[2].isNull() ? profile[2].get<double>() : 0.0);
        const double hourly = existing && !existing[4].isNull() ? existing[4].get<double>()
                              : (profile && !profile[3].isNull() ? profile[3].get<double>() : 0.0);
        const double oldHours = existing ? existing[5].get<double>() : 0.0;
        const double oldAttendance = existing ? existing[6].get<double>() : 0.0;
        const double oldPerformance = existing ? existing[7].get<double>() : 0.0;
        const double oldAllowance = existing ? existing[8].get<double>() : 0.0;
        const double oldDeduction = existing ? existing[9].get<double>() : 0.0;
        const double oldSocial = existing ? existing[10].get<double>()
                              : (profile ? profile[4].get<double>() : 0.0);
        const double hours = getRequestDoubleWithFallback(*body, "work_hours_month", "workHoursMonth", oldHours);
        const double attendance = getRequestDoubleWithFallback(*body, "attendance_award", "attendanceAward", oldAttendance);
        const double performance = getRequestDoubleWithFallback(*body, "performance_award", "performanceAward", oldPerformance);
        const double allowance = getRequestDoubleWithFallback(*body, "allowance", "allowance", oldAllowance);
        const double deduction = getRequestDoubleWithFallback(*body, "deduction", "deduction", oldDeduction);
        const double social = getRequestDoubleWithFallback(*body, "social_insurance_housing_fund", "socialInsuranceHousingFund", oldSocial);
        if ((payType != "monthly" && payType != "hourly") ||
            !std::isfinite(base) || !std::isfinite(hourly) || !std::isfinite(social) || !std::isfinite(hours) ||
            !std::isfinite(attendance) || !std::isfinite(performance) || !std::isfinite(allowance) || !std::isfinite(deduction) ||
            base < 0 || hourly < 0 || social < 0 || hours < 0 || attendance < 0 || performance < 0 || allowance < 0 || deduction < 0)
        { rollbackTransactionQuietly(*session); return ResponseHelper::validation(req, "工资字段值无效"); }
        const double total = std::round(((payType == "hourly" ? hourly * hours : base) + attendance + performance + allowance - deduction - social) * 100.0) / 100.0;
        if (!std::isfinite(total) || total < 0)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "实发工资不能为负数");
        }
        const double oldTotal = existing ? existing[11].get<double>() : 0.0;
        const bool changed = !existing || std::abs(oldHours - hours) > 0.004 ||
            std::abs(oldAttendance - attendance) > 0.004 || std::abs(oldPerformance - performance) > 0.004 ||
            std::abs(oldAllowance - allowance) > 0.004 || std::abs(oldDeduction - deduction) > 0.004 ||
            std::abs(oldSocial - social) > 0.004 || std::abs(oldTotal - total) > 0.004;
        if (!changed)
        {
            session->sql("COMMIT").execute();
            return ResponseHelper::success(req, "工资快照未发生变化");
        }

        auto upsert = session->sql("INSERT INTO salary (payroll_period_id,salary_profile_id,user_id,pay_type,base_salary,hourly_rate,work_hours_month,attendance_award,performance_award,allowance,deduction,social_insurance_housing_fund,total_salary,review_status,is_manually_modified) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,'pending',?) ON DUPLICATE KEY UPDATE salary_profile_id=VALUES(salary_profile_id),pay_type=VALUES(pay_type),base_salary=VALUES(base_salary),hourly_rate=VALUES(hourly_rate),work_hours_month=VALUES(work_hours_month),attendance_award=VALUES(attendance_award),performance_award=VALUES(performance_award),allowance=VALUES(allowance),deduction=VALUES(deduction),social_insurance_housing_fund=VALUES(social_insurance_housing_fund),total_salary=VALUES(total_salary),is_manually_modified=IF(?=1,1,is_manually_modified),review_status=IF(review_status='locked',review_status,'pending')")
            .bind(periodId, profileId > 0 ? mysqlx::Value(profileId) : mysqlx::Value(), goalUserId, payType,
                  payType == "monthly" ? mysqlx::Value(base) : mysqlx::Value(),
                  payType == "hourly" ? mysqlx::Value(hourly) : mysqlx::Value(),
                  hours, attendance, performance, allowance, deduction, social, total, 1, 1);
        upsert.execute();
        const auto salary = session->sql("SELECT id FROM salary WHERE payroll_period_id=? AND user_id=?")
            .bind(periodId, goalUserId).execute().fetchOne();
        if (!salary)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::error(req, "工资快照保存成功但修改记录创建失败");
        }
        const long long salaryId = salary[0].get<long long>();
        const std::string evidencePath = body->value("evidence_path", body->value("evidencePath", std::string{}));
        const auto recordChange = [&](const std::string &field, double before, double after)
        {
            if (!existing || std::abs(before - after) > 0.004)
            {
                session->sql("INSERT INTO salaryChangeRecord (salary_id,changed_field,before_value,after_value,changed_by,change_reason,evidence_path) VALUES (?,?,?,?,?,?,?)")
                    .bind(salaryId, field, existing ? mysqlx::Value(moneyValue(before)) : mysqlx::Value(),
                          moneyValue(after), operatorId, changeReason, evidencePath).execute();
            }
        };
        recordChange("work_hours_month", oldHours, hours);
        recordChange("attendance_award", oldAttendance, attendance);
        recordChange("performance_award", oldPerformance, performance);
        recordChange("allowance", oldAllowance, allowance);
        recordChange("deduction", oldDeduction, deduction);
        recordChange("social_insurance_housing_fund", oldSocial, social);
        recordChange("total_salary", oldTotal, total);
        refreshPayrollPeriodTotal(*session, periodId);
        session->sql("COMMIT").execute();
        FinanceHomeDataBroadcaster::instance().notifyHomeDataChanged();
        return ResponseHelper::success(req, "工资快照已保存");
    }
    catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}

crow::response financeHandler::saveSalaryProfile(const crow::request &req, int goalUserId)
{
    try
    {
        crow::response res;
        auto body = validateRequest(req, res);
        if (!body) return res;
        if (goalUserId <= 0) return ResponseHelper::validation(req, "无效的用户ID");

        const std::string payType = body->value("pay_type", body->value("payType", "monthly"));
        const double base = getRequestDoubleWithFallback(*body, "base_salary", "baseSalary", 0.0);
        const double hourly = getRequestDoubleWithFallback(*body, "hourly_rate", "hourlyRate", 0.0);
        const double social = getRequestDoubleWithFallback(*body, "social_insurance_housing_fund", "socialInsuranceHousingFund", 0.0);
        const std::string effectiveFrom = body->value("effective_from", body->value("effectiveFrom", std::string{}));
        bool validEffectiveDate = true;
        try
        {
            validEffectiveDate = boost::gregorian::to_iso_extended_string(
                                     boost::gregorian::from_simple_string(effectiveFrom)) == effectiveFrom;
        }
        catch (const std::exception &)
        {
            validEffectiveDate = false;
        }
        if ((payType != "monthly" && payType != "hourly") || !validEffectiveDate || effectiveFrom.size() != 10 ||
            effectiveFrom[4] != '-' || effectiveFrom[7] != '-' || !std::isfinite(base) || !std::isfinite(hourly) ||
            !std::isfinite(social) || base < 0 || hourly < 0 || social < 0)
        {
            return ResponseHelper::validation(req, "薪资配置字段值无效");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();
        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");
        const auto employee = session->sql(
            "SELECT u.id FROM users u LEFT JOIN positions pos ON pos.id=u.position_id "
            "WHERE u.id=? AND u.is_deleted=0 AND u.account_type='staff' " + scopeFilter + " LIMIT 1 FOR UPDATE")
            .bind(goalUserId).execute().fetchOne();
        if (!employee)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::notFound(req, "员工不存在");
        }

        session->sql("SELECT id FROM salaryProfile WHERE user_id=? FOR UPDATE").bind(goalUserId).execute();
        session->sql(
            "INSERT INTO salaryProfile (user_id,pay_type,base_salary,hourly_rate,social_insurance_housing_fund,effective_from) "
            "VALUES (?,?,?,?,?,?) ON DUPLICATE KEY UPDATE pay_type=VALUES(pay_type),base_salary=VALUES(base_salary),"
            "hourly_rate=VALUES(hourly_rate),social_insurance_housing_fund=VALUES(social_insurance_housing_fund)")
            .bind(goalUserId, payType,
                  payType == "monthly" ? mysqlx::Value(base) : mysqlx::Value(),
                  payType == "hourly" ? mysqlx::Value(hourly) : mysqlx::Value(), social, effectiveFrom)
            .execute();

        std::vector<std::pair<int, std::string>> profiles;
        auto profileRows = session->sql(
            "SELECT id,CAST(effective_from AS CHAR) FROM salaryProfile WHERE user_id=? "
            "ORDER BY effective_from ASC,id ASC FOR UPDATE").bind(goalUserId).execute();
        for (auto row : profileRows)
        {
            profiles.emplace_back(row[0].get<int>(), row[1].get<std::string>());
        }
        for (std::size_t i = 0; i < profiles.size(); ++i)
        {
            if (i + 1 < profiles.size())
            {
                session->sql("UPDATE salaryProfile SET effective_to=DATE_SUB(?,INTERVAL 1 DAY) WHERE id=?")
                    .bind(profiles[i + 1].second, profiles[i].first).execute();
            }
            else
            {
                session->sql("UPDATE salaryProfile SET effective_to=NULL WHERE id=?")
                    .bind(profiles[i].first).execute();
            }
        }
        session->sql("COMMIT").execute();
        return ResponseHelper::success(req, "薪资配置已保存，不影响已有工资快照");
    }
    catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}

crow::response financeHandler::getSalaryProfile(const crow::request &req, int goalUserId)
{
    try
    {
        if (goalUserId <= 0) return ResponseHelper::validation(req, "无效的用户ID");
        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");
        auto session = dbManager->getSession();
        const auto row = session->sql(
            "SELECT sp.id,sp.pay_type,sp.base_salary,sp.hourly_rate,sp.social_insurance_housing_fund,"
            "CAST(sp.effective_from AS CHAR),CAST(sp.effective_to AS CHAR) "
            "FROM salaryProfile sp JOIN users u ON u.id=sp.user_id LEFT JOIN positions pos ON pos.id=u.position_id "
            "WHERE sp.user_id=? AND u.is_deleted=0 AND sp.effective_to IS NULL " + scopeFilter +
            " ORDER BY sp.effective_from DESC,sp.id DESC LIMIT 1")
            .bind(goalUserId).execute().fetchOne();
        if (!row) return ResponseHelper::notFound(req, "生效薪资配置未找到");
        return ResponseHelper::success(req, {
            {"id", row[0].get<int>()}, {"user_id", goalUserId}, {"pay_type", row[1].get<std::string>()},
            {"base_salary", row[2].isNull() ? 0.0 : row[2].get<double>()},
            {"hourly_rate", row[3].isNull() ? 0.0 : row[3].get<double>()},
            {"social_insurance_housing_fund", row[4].isNull() ? 0.0 : row[4].get<double>()},
            {"effective_from", row[5].isNull() ? "" : row[5].get<std::string>()},
            {"effective_to", row[6].isNull() ? "" : row[6].get<std::string>()}});
    }
    catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}

crow::response financeHandler::getSalaryChangeHistory(const crow::request &req, long long salaryId)
{
    try
    {
        if (salaryId <= 0) return ResponseHelper::validation(req, "无效的工资单编号");
        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");
        auto session = dbManager->getSession();
        const auto visible = session->sql(
            "SELECT s.id FROM salary s JOIN users u ON u.id=s.user_id "
            "LEFT JOIN positions pos ON pos.id=u.position_id WHERE s.id=? AND u.is_deleted=0 " + scopeFilter)
            .bind(salaryId).execute().fetchOne();
        if (!visible) return ResponseHelper::notFound(req, "工资单未找到");
        nlohmann::json items = nlohmann::json::array();
        auto result = session->sql(
            "SELECT cr.id,cr.changed_field,cr.before_value,cr.after_value,cr.changed_by,"
            "COALESCE(u.name,''),cr.change_reason,cr.evidence_path,CAST(cr.created_at AS CHAR) "
            "FROM salaryChangeRecord cr LEFT JOIN users u ON u.id=cr.changed_by "
            "WHERE cr.salary_id=? ORDER BY cr.created_at DESC,cr.id DESC")
            .bind(salaryId).execute();
        for (auto row : result)
        {
            items.push_back({
                {"id", row[0].get<long long>()}, {"changed_field", row[1].get<std::string>()},
                {"before_value", row[2].isNull() ? "" : row[2].get<std::string>()},
                {"after_value", row[3].isNull() ? "" : row[3].get<std::string>()},
                {"changed_by", row[4].isNull() ? 0 : row[4].get<int>()},
                {"changed_by_name", row[5].isNull() ? "" : clean_string(row[5].get<std::string>())},
                {"change_reason", row[6].isNull() ? "" : row[6].get<std::string>()},
                {"evidence_path", row[7].isNull() ? "" : row[7].get<std::string>()},
                {"created_at", row[8].isNull() ? "" : row[8].get<std::string>()}});
        }
        return ResponseHelper::success(req, {{"items", items}});
    }
    catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}

crow::response financeHandler::reviewPayrollEmployee(const crow::request &req, int goalUserId)
{
    try
    {
        if (goalUserId <= 0) return ResponseHelper::validation(req, "无效的用户ID");
        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();
        const int periodId = ensureCurrentPayrollPeriod(*session);
        const std::string scopeFilter = orgScopeCondition(req, dbManager, "pos.department_id");
        const auto employee = session->sql(
            "SELECT s.id,p.status FROM salary s JOIN users u ON u.id=s.user_id "
            "LEFT JOIN positions pos ON pos.id=u.position_id JOIN payrollPeriod p ON p.id=s.payroll_period_id "
            "WHERE s.user_id=? AND s.payroll_period_id=? AND u.is_deleted=0 " + scopeFilter + " FOR UPDATE")
            .bind(goalUserId, periodId).execute().fetchOne();
        if (!employee)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::notFound(req, "当前周期工资快照未找到");
        }
        if (employee[1].get<std::string>() != "first_review")
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "当前周期不在财务初审阶段");
        }
        const auto result = session->sql("UPDATE salary SET review_status='first_reviewed' WHERE id=? AND review_status='pending'")
            .bind(employee[0].get<long long>()).execute();
        if (result.getAffectedItemsCount() != 1)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "该员工工资已初审或状态已变化");
        }
        session->sql("COMMIT").execute();
        return ResponseHelper::success(req, "员工工资已完成初审");
    }
    catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}

crow::response financeHandler::submitPayrollReview(const crow::request &req)
{
    try
    {
        if (!hasUnrestrictedOrgScope(req, dbManager))
        {
            return ResponseHelper::permission_denied(req, "提交全局工资复审需要全部组织数据范围");
        }
        nlohmann::json body = nlohmann::json::object();
        if (!req.body.empty())
        {
            body = nlohmann::json::parse(req.body, nullptr, false);
            if (body.is_discarded() || !body.is_object())
            {
                return ResponseHelper::validation(req, "复审说明格式无效");
            }
        }
        const std::string reviewNote = body.value("reviewNote", body.value("review_note", std::string{}));
        if (reviewNote.size() > 1000)
        {
            return ResponseHelper::validation(req, "复审说明不能超过1000个字符");
        }
        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();
        const int periodId = ensureCurrentPayrollPeriod(*session);
        const auto period = session->sql("SELECT status FROM payrollPeriod WHERE id=? FOR UPDATE")
            .bind(periodId).execute().fetchOne();
        if (!period || period[0].get<std::string>() != "first_review")
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "当前工资周期不允许提交复审");
        }
        const auto blockers = session->sql(
            "SELECT COUNT(*) FROM users u JOIN payrollPeriod p ON p.id=? "
            "LEFT JOIN salaryProfile sp ON sp.id=(SELECT sp2.id FROM salaryProfile sp2 WHERE sp2.user_id=u.id "
            "AND sp2.effective_from<=p.payroll_month "
            "AND (sp2.effective_to IS NULL OR sp2.effective_to>=p.payroll_month) "
            "ORDER BY sp2.effective_from DESC,sp2.id DESC LIMIT 1) "
            "LEFT JOIN salary s ON s.user_id=u.id AND s.payroll_period_id=p.id "
            "WHERE u.account_type='staff' AND u.is_deleted=0 "
            "AND (sp.id IS NULL OR s.id IS NULL OR s.review_status<>'first_reviewed')")
            .bind(periodId).execute().fetchOne();
        const auto staffCount = session->sql("SELECT COUNT(*) FROM users WHERE account_type='staff' AND is_deleted=0")
            .execute().fetchOne();
        if (!staffCount || staffCount[0].get<int>() == 0 || (blockers && blockers[0].get<int>() > 0))
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "仍有员工缺少当前薪资配置、工资快照或未完成初审");
        }
        const auto result = session->sql(
            "UPDATE payrollPeriod SET status='second_review',reviewed_by=?,reviewed_at=NOW(),review_note=? "
            "WHERE id=? AND status='first_review'")
            .bind(currentRequestUserId(req).value_or(0), reviewNote, periodId).execute();
        if (result.getAffectedItemsCount() != 1)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "当前工资周期不允许提交复审");
        }
        session->sql("UPDATE salary SET review_status='second_reviewed' WHERE payroll_period_id=? AND review_status='first_reviewed'")
            .bind(periodId).execute();
        session->sql("COMMIT").execute();
        return ResponseHelper::success(req, "已提交主管复审");
    }
    catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}

crow::response financeHandler::lockPayroll(const crow::request &req)
{
    try
    {
        if (!hasUnrestrictedOrgScope(req, dbManager))
        {
            return ResponseHelper::permission_denied(req, "锁定全局工资周期需要全部组织数据范围");
        }
        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();
        const int periodId = ensureCurrentPayrollPeriod(*session);
        const auto period = session->sql("SELECT status FROM payrollPeriod WHERE id=? FOR UPDATE")
            .bind(periodId).execute().fetchOne();
        if (!period || period[0].get<std::string>() != "second_review")
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "当前工资周期尚未进入主管复审阶段");
        }
        const auto blockers = session->sql(
            "SELECT COUNT(*) FROM users u LEFT JOIN salary s ON s.user_id=u.id AND s.payroll_period_id=? "
            "WHERE u.account_type='staff' AND u.is_deleted=0 "
            "AND (s.id IS NULL OR s.review_status<>'second_reviewed')")
            .bind(periodId).execute().fetchOne();
        if (blockers && blockers[0].get<int>() > 0)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "仍有工资记录未完成复审");
        }
        const auto result = session->sql("UPDATE payrollPeriod SET status='locked',locked_by=?,locked_at=NOW() WHERE id=? AND status='second_review'")
            .bind(currentRequestUserId(req).value_or(0), periodId).execute();
        if (result.getAffectedItemsCount() != 1)
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "当前工资周期尚未完成复审");
        }
        session->sql("UPDATE salary SET review_status='locked' WHERE payroll_period_id=?").bind(periodId).execute();
        refreshPayrollPeriodTotal(*session, periodId);
        session->sql("COMMIT").execute();
        return ResponseHelper::success(req, "工资周期已锁定");
    }
    catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}

crow::response financeHandler::createPayrollRevision(const crow::request &req)
{
    try
    {
        if (!hasUnrestrictedOrgScope(req, dbManager))
        {
            return ResponseHelper::permission_denied(req, "创建全局工资修订版需要全部组织数据范围");
        }
        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();
        const auto latest = session->sql(
            "SELECT id,status,version_no FROM payrollPeriod "
            "WHERE payroll_month=DATE_FORMAT(CURDATE(), '%Y-%m-01') "
            "ORDER BY version_no DESC LIMIT 1 FOR UPDATE").execute().fetchOne();
        if (!latest || (latest[1].get<std::string>() != "locked" && latest[1].get<std::string>() != "archived"))
        {
            rollbackTransactionQuietly(*session);
            return ResponseHelper::validation(req, "仅已锁定或已归档周期可创建同月修订版");
        }
        const int sourcePeriodId = latest[0].get<int>();
        const int nextVersion = latest[2].get<int>() + 1;
        auto inserted = session->sql(
            "INSERT INTO payrollPeriod (payroll_month,status,version_no) "
            "VALUES (DATE_FORMAT(CURDATE(), '%Y-%m-01'),'first_review',?)")
            .bind(nextVersion).execute();
        const int revisionId = static_cast<int>(inserted.getAutoIncrementValue());
        session->sql(
            "INSERT INTO salary (payroll_period_id,salary_profile_id,user_id,pay_type,base_salary,hourly_rate,"
            "work_hours_month,attendance_award,performance_award,allowance,deduction,social_insurance_housing_fund,"
            "total_salary,review_status,is_manually_modified) "
            "SELECT ?,salary_profile_id,user_id,pay_type,base_salary,hourly_rate,work_hours_month,attendance_award,"
            "performance_award,allowance,deduction,social_insurance_housing_fund,total_salary,'pending',0 "
            "FROM salary WHERE payroll_period_id=?")
            .bind(revisionId, sourcePeriodId).execute();
        refreshPayrollPeriodTotal(*session, revisionId);
        session->sql("COMMIT").execute();
        FinanceHomeDataBroadcaster::instance().notifyHomeDataChanged();
        return ResponseHelper::success(req, {{"periodId", revisionId}, {"versionNo", nextVersion}});
    }
    catch (const std::exception &e) { return ResponseHelper::system_error(req, e.what()); }
}
