#include "financeHandler.h"

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

crow::response financeHandler::homePageGetData(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        double salesCount = calculateSalesCount();
        double costCount = calculateCostCount();
        adminHandler adminer(dbManager);

        nlohmann::json summary = {
            {"userCount", adminer.calculateUserCount()},
            {"onlineDoctorCount", adminer.calculateOnlineDoctorCount()},
            {"logCount", adminer.calculateLogsCount()},
            {"salesCount", salesCount},
            {"costCount", costCount},
            {"profitCount", salesCount - costCount}};

        return ResponseHelper::success(req, summary);
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
                                            .bind()
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
            {"page_size", pageSize}
        };

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

        if(salaryId <= 0)
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
        if(!row)
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

crow::response financeHandler::getSalaryManagementData(const crow::request &req)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        // 获取员工列表及其工资信息
        mysqlx::SqlResult employeesResult = dbManager->getSession()
                                                ->sql("SELECT u.id, u.type_id, t.type, u.name, u.phone, u.email, "
                                                      "COALESCE(s.base_salary, 0), COALESCE(s.PA_Award, 0), "
                                                      "COALESCE(s.PB_Award, 0), COALESCE(s.total_salary, 0), "
                                                      "CAST(s.updated_at AS CHAR) "
                                                      "FROM users AS u "
                                                      "LEFT JOIN types AS t ON u.type_id = t.id "
                                                      "LEFT JOIN salary AS s ON s.user_id = u.id "
                                                      "WHERE u.type_id <> ? AND u.is_deleted = 0 "
                                                      "ORDER BY COALESCE(s.total_salary, 0) DESC, u.id ASC")
                                                .bind(normalUserRoleId)
                                                .execute();

        // 获取最近6条月度记录
        mysqlx::SqlResult monthlyRecordsResult = dbManager->getSession()
                                                     ->sql("SELECT CAST(id AS CHAR), salesCount, costCount, profitCount, "
                                                           "CAST(created_at AS CHAR) "
                                                           "FROM salaryRecord "
                                                           "WHERE record_type = 'month' "
                                                           "ORDER BY created_at DESC "
                                                           "LIMIT 6")
                                                     .execute();

        // 获取最近10条日常记录
        mysqlx::SqlResult dailyRecordsResult = dbManager->getSession()
                                                   ->sql("SELECT CAST(id AS CHAR), salesCount, costCount, profitCount, "
                                                         "CAST(created_at AS CHAR) "
                                                         "FROM monthlySalaryRecord "
                                                         "ORDER BY created_at DESC "
                                                         "LIMIT 10")
                                                   .execute();

        nlohmann::json response;
        response["employees"] = nlohmann::json::array();
        response["monthlyRecords"] = nlohmann::json::array();
        response["dailyRecords"] = nlohmann::json::array();

        double totalPayroll = 0.0;
        int employeeCount = 0;

        for (auto row : employeesResult)
        {
            nlohmann::json employee;
            const double baseSalary = row[6].isNull() ? 0.0 : row[6].get<double>();
            const double paAward = row[7].isNull() ? 0.0 : row[7].get<double>();
            const double pbAward = row[8].isNull() ? 0.0 : row[8].get<double>();
            const double totalSalary = row[9].isNull() ? 0.0 : row[9].get<double>();

            employee["id"] = row[0].isNull() ? 0 : row[0].get<int>();
            employee["type_id"] = row[1].isNull() ? nullptr : nlohmann::json(row[1].get<int>());
            employee["type_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
            employee["name"] = row[3].isNull() ? "" : clean_string(row[3].get<std::string>());
            employee["phone"] = row[4].isNull() ? "" : clean_string(row[4].get<std::string>());
            employee["email"] = row[5].isNull() ? "" : clean_string(row[5].get<std::string>());
            employee["base_salary"] = baseSalary;
            employee["pa_award"] = paAward;
            employee["pb_award"] = pbAward;
            employee["total_salary"] = totalSalary;
            employee["updated_at"] = row[10].isNull() ? "" : row[10].get<std::string>();

            totalPayroll += totalSalary;
            employeeCount += 1;
            response["employees"].push_back(employee);
        }

        for (auto row : monthlyRecordsResult)
        {
            nlohmann::json record;
            record["id"] = row[0].isNull() ? "" : row[0].get<std::string>();
            record["salesCount"] = row[1].isNull() ? 0.0 : row[1].get<double>();
            record["costCount"] = row[2].isNull() ? 0.0 : row[2].get<double>();
            record["profitCount"] = row[3].isNull() ? 0.0 : row[3].get<double>();
            record["created_at"] = row[4].isNull() ? "" : row[4].get<std::string>();
            response["monthlyRecords"].push_back(record);
        }

        for (auto row : dailyRecordsResult)
        {
            nlohmann::json record;
            record["id"] = row[0].isNull() ? "" : row[0].get<std::string>();
            record["salesCount"] = row[1].isNull() ? 0.0 : row[1].get<double>();
            record["costCount"] = row[2].isNull() ? 0.0 : row[2].get<double>();
            record["profitCount"] = row[3].isNull() ? 0.0 : row[3].get<double>();
            record["created_at"] = row[4].isNull() ? "" : row[4].get<std::string>();
            response["dailyRecords"].push_back(record);
        }

        double salesCount = calculateSalesCount();
        double costCount = calculateCostCount();

        response["summary"] = {
            {"employeeCount", employeeCount},
            {"monthlyPayroll", totalPayroll},
            {"todayCost", costCount},
            {"todayProfit", salesCount - costCount}};

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response financeHandler::changeSalary(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userId = request_body.value("userId", 0);
        double baseSalary = request_body.value("baseSalary", 0.0);
        double paAward = request_body.value("paAward", 0.0);
        double pbAward = request_body.value("pbAward", 0.0);
        double totalSalary = baseSalary + paAward + pbAward;

        if (userId == 0)
        {
            return ResponseHelper::unavailable(req, "用户ID不能为空");
        }
        if (baseSalary < 0 || paAward < 0 || pbAward < 0)
        {
            return ResponseHelper::unavailable(req, "工资项不能小于0");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            mysqlx::SqlResult salaryRowResult = session->sql("SELECT id FROM salary WHERE user_id = ? LIMIT 1")
                                                    .bind(userId)
                                                    .execute();

            mysqlx::Row salaryRow = salaryRowResult.fetchOne();

            if (salaryRow && !salaryRow[0].isNull())
            {
                mysqlx::SqlResult updateResult = session->sql("UPDATE salary SET base_salary = ?, PA_Award = ?, PB_Award = ?, total_salary = ? WHERE user_id = ?")
                                                     .bind(baseSalary, paAward, pbAward, totalSalary, userId)
                                                     .execute();
                if (updateResult.getAffectedItemsCount() != 1)
                {
                    session->sql("ROLLBACK").execute();
                    return ResponseHelper::operation_failed(req, "工资修改失败");
                }
            }
            else
            {
                mysqlx::SqlResult insertResult = session->sql("INSERT INTO salary (user_id, base_salary, PA_Award, PB_Award, total_salary) "
                                                              "VALUES (?, ?, ?, ?, ?)")
                                                     .bind(userId, baseSalary, paAward, pbAward, totalSalary)
                                                     .execute();

                if (insertResult.getAffectedItemsCount() != 1)
                {
                    session->sql("ROLLBACK").execute();
                    return ResponseHelper::operation_failed(req, "工资创建失败");
                }
            }

            session->sql("COMMIT").execute();
        }
        catch (...)
        {
            session->sql("ROLLBACK").execute();
            throw;
        }

        return ResponseHelper::success(req, "工资修改成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}
