#include "financeHandler.h"
#include "../admin/adminHandler.h"
#include "../../../utils/RoleTypeUtils/RoleTypeUtils.h"

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
