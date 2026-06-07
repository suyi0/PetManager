#include "financeHandler.h"
#include <cmath>

namespace
{
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

        const int normalUserRoleId =
            RoleTypeUtils::getRoleId(dbManager, "普通用户");
        if (normalUserRoleId <= 0)
        {
            throw std::runtime_error("Failed to resolve normal user role id");
        }

        const auto [dayStart, nextDayStart] = getTodayRange();

        const double employeeCostCount = dbManager->getSession()
                                             ->sql("SELECT COALESCE(ROUND(SUM(s.total_salary / 31)), 0) "
                                                   "FROM salary AS s "
                                                   "JOIN users AS u ON u.id = s.user_id "
                                                   "WHERE u.type_id <> ?")
                                             .bind(normalUserRoleId)
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

crow::response financeHandler::getHomeData(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        // 获取每日员工工资开销记录
        mysqlx::SqlResult dailyExpensesResult = dbManager->getSession()
                                                    ->sql("SELECT COALESCE(ROUND(SUM(total_salary / 31)), 0)  "
                                                          "FROM salary ")
                                                    .execute();

        auto dailyRow = dailyExpensesResult.fetchOne();

        double salesCount = calculateSalesCount();
        double costCount = calculateCostCount();

        nlohmann::json data = {
            {"dailyExpense", dailyRow[0].isNull() ? 0.0 : dailyRow[0].get<double>()},
            {"dailyCost", costCount},
            {"dailySales", salesCount},
            {"dailyProfit", salesCount - costCount}};

        return ResponseHelper::success(req, data);
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

        // 验证参数
        for (const std::string &key : {"baseSalary", "base_salary", "paAward", "PA_Award", "pbAward", "PB_Award"})
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
            mysqlx::SqlResult employeeResult = session->sql(
                                                          "SELECT u.name, t.type, s.id, s.base_salary, s.PA_Award, s.PB_Award "
                                                          "FROM users AS u "
                                                          "JOIN types AS t ON t.id = u.type_id "
                                                          "LEFT JOIN salary AS s ON s.user_id = u.id "
                                                          "WHERE u.id = ? AND u.is_deleted = 0 "
                                                          "LIMIT 1 FOR UPDATE")
                                                   .bind(goalUserId)
                                                   .execute();

            auto employeeRow = employeeResult.fetchOne();
            if (!employeeRow)
            {
                session->sql("ROLLBACK").execute();
                return ResponseHelper::notFound(req, "员工不存在");
            }

            const std::string employeeName = employeeRow[0].isNull() ? "" : clean_string(employeeRow[0].get<std::string>());
            const std::string employeeType = employeeRow[1].isNull() ? "" : employeeRow[1].get<std::string>();
            if (RoleTypeUtils::isNormalUserRole(employeeType)) // 普通用户没有工资
            {
                session->sql("ROLLBACK").execute();
                return ResponseHelper::validation(req, "普通用户不能创建员工工资记录");
            }

            const bool isNewSalary = employeeRow[2].isNull();
            const int salaryId = isNewSalary ? 0 : employeeRow[2].get<int>();
            const double DB_BaseSalary = employeeRow[3].isNull() ? 0.0 : employeeRow[3].get<double>();
            const double DB_PA_Award = employeeRow[4].isNull() ? 0.0 : employeeRow[4].get<double>();
            const double DB_PB_Award = employeeRow[5].isNull() ? 0.0 : employeeRow[5].get<double>();

            const double rawBaseSalary = getRequestDoubleWithFallback(request_body, "baseSalary", "base_salary", DB_BaseSalary);
            const double rawPA_Award = getRequestDoubleWithFallback(request_body, "PAAward", "PA_Award", DB_PA_Award);
            const double rawPB_Award = getRequestDoubleWithFallback(request_body, "PBAward", "PB_Award", DB_PB_Award);

            // 判断前端传输的工资参数是有限值
            if (!std::isfinite(rawBaseSalary) || !std::isfinite(rawPA_Award) ||
                !std::isfinite(rawPB_Award) ||
                rawBaseSalary < 0 || rawPA_Award < 0 || rawPB_Award < 0)
            {
                session->sql("ROLLBACK").execute();
                return ResponseHelper::validation(req, "工资金额必须为大于或等于零的有效数字");
            }

            const double baseSalary = std::round(rawBaseSalary * 100.0) / 100.0;
            const double PA_Award = std::round(rawPA_Award * 100.0) / 100.0;
            const double PB_Award = std::round(rawPB_Award * 100.0) / 100.0;
            const double totalSalary = std::round((baseSalary + PA_Award + PB_Award) * 100.0) / 100.0;

            if (!std::isfinite(totalSalary))
            {
                session->sql("ROLLBACK").execute();
                return ResponseHelper::validation(req, "工资总额超出有效范围");
            }

            const bool hasChanges = baseSalary != DB_BaseSalary ||
                                    PA_Award != DB_PA_Award ||
                                    PB_Award != DB_PB_Award;

            if (!isNewSalary && !hasChanges)
            {
                session->sql("COMMIT").execute();
                return ResponseHelper::success(req, "员工工资记录已存在，并且没有需要更新的字段");
            }

            int savedSalaryId = salaryId;
            if (isNewSalary) // 添加新员工工资
            {
                mysqlx::SqlResult insertResult = session->sql(
                                                            "INSERT INTO salary (user_id, base_salary, PA_Award, PB_Award, total_salary) "
                                                            "VALUES (?, ?, ?, ?, ?)")
                                                     .bind(goalUserId, baseSalary, PA_Award, PB_Award, totalSalary)
                                                     .execute();

                if (insertResult.getAffectedItemsCount() != 1)
                {
                    session->sql("ROLLBACK").execute();
                    return ResponseHelper::error(req, "给新员工添加工资失败");
                }
                savedSalaryId = static_cast<int>(insertResult.getAutoIncrementValue());
            }
            else // 更新员工工资
            {
                mysqlx::SqlResult updateResult = session->sql(
                                                            "UPDATE salary SET base_salary = ?, PA_Award = ?, PB_Award = ?, total_salary = ? "
                                                            "WHERE id = ? AND user_id = ?")
                                                     .bind(baseSalary, PA_Award, PB_Award, totalSalary, salaryId, goalUserId)
                                                     .execute();

                if (updateResult.getAffectedItemsCount() != 1)
                {
                    session->sql("ROLLBACK").execute();
                    return ResponseHelper::error(req, "更新员工工资失败");
                }
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
            return isNewSalary ? ResponseHelper::created(req, data) : ResponseHelper::success(req, data);
        }
        catch (const std::invalid_argument &e)
        {
            session->sql("ROLLBACK").execute();
            return ResponseHelper::validation(req, e.what());
        }
        catch (...)
        {
            session->sql("ROLLBACK").execute();
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

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT s.id, u.name, t.type, COALESCE(s.total_salary, 0) "
                                             "FROM salary AS s "
                                             "JOIN users AS u ON u.id = s.user_id "
                                             "JOIN types AS t ON t.id = u.type_id "
                                             "WHERE u.is_deleted = 0 "
                                             "ORDER BY COALESCE(s.total_salary, 0) DESC, u.id ASC "
                                             "LIMIT ?, ?")
                                       .bind(offset, pageSize)
                                       .execute();

        mysqlx::SqlResult countResult = dbManager->getSession()
                                            ->sql("SELECT COUNT(*) "
                                                  "FROM salary AS s "
                                                  "JOIN users AS u ON u.id = s.user_id "
                                                  "WHERE u.is_deleted = 0")
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

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT u.id, s.id, u.name, t.type, s.base_salary, s.PA_Award, s.PB_Award, s.total_salary, CAST(s.update_at AS CHAR) "
                                             "FROM salary AS s "
                                             "JOIN users AS u ON u.id = s.user_id "
                                             "JOIN types AS t ON t.id = u.type_id "
                                             "WHERE s.id = ? AND u.is_deleted = 0")
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
        data["base_salary"] = row[4].isNull() ? 0.0 : row[4].get<double>();
        data["PA_Award"] = row[5].isNull() ? 0.0 : row[5].get<double>();
        data["PB_Award"] = row[6].isNull() ? 0.0 : row[6].get<double>();
        data["total_salary"] = row[7].isNull() ? 0.0 : row[7].get<double>();
        data["updated_at"] = row[8].isNull() ? "" : row[8].get<std::string>();

        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "财务", "获取员工工资详情", "Failed to get salary information: " + std::string(e.what()));
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
                                                            "FROM monthlySalaryRecord "
                                                            "ORDER BY created_at DESC "
                                                            "LIMIT 10")
                                                   .execute();

        // 获取最近10条月度记录
        mysqlx::SqlResult monthlyRecordsResult = session->sql("SELECT id, salesCount, costCount, profitCount, "
                                                              "CAST(created_at AS CHAR) "
                                                              "FROM salaryRecord "
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
