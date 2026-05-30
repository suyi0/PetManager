#include "reservationCommonHandler.h"
#include "RoleTypeUtils/RoleTypeUtils.h"

crow::response reservationCommonHandler::getReservationSummary(const crow::request &req, int userId)
{
    if (!checkDbConnection())
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取预约列表", "database connection failed", userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
    }

    try
    {
        const std::string roleName = RoleTypeUtils::getUserRoleName(dbManager, userId);
        const bool isBoss = RoleTypeUtils::isBossRole(roleName);
        const bool isMedicalStaff = RoleTypeUtils::isMedicalStaffRole(roleName);
        const std::string filterSql = isBoss
                                          ? ""
                                      : isMedicalStaff ? "WHERE r.doctor_id = ? "
                                                       : "WHERE r.user_id = ? AND r.user_hidden = 0 ";

        const std::string sql = "SELECT r.id, p.pet_name, d.name, "
                                "CAST(r.date AS CHAR), COALESCE(r.time_slot, ''), "
                                "COALESCE(r.reservation_type, ''), COALESCE(r.status, '') "
                                "FROM reaservations AS r "
                                "LEFT JOIN pets AS p ON r.pet_id = p.id "
                                "LEFT JOIN users AS d ON r.doctor_id = d.id " +
                                filterSql;

        auto query = dbManager->getSession()->sql(sql);
        if (!isBoss)
        {
            query.bind(userId);
        }

        mysqlx::SqlResult result = query.execute();

        nlohmann::json data = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json reservationSummary;
            reservationSummary["id"] = row[0].get<int>();
            reservationSummary["pet_name"] = row[1].isNull() ? "" : row[1].get<std::string>();
            reservationSummary["doctor_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
            reservationSummary["date"] = row[3].isNull() ? "" : row[3].get<std::string>();
            reservationSummary["time_slot"] = row[4].isNull() ? "" : row[4].get<std::string>();
            reservationSummary["reservation_type"] = row[5].isNull() ? "" : row[5].get<std::string>();
            reservationSummary["status"] = row[6].isNull() ? "预约成功" : row[6].get<std::string>();
            data.push_back(reservationSummary);
        }

        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取预约列表", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::system_error(req, e.what());
    }
}

nlohmann::json reservationCommonHandler::getReservationData(const int &reservationId)
{
    try
    {

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT r.id, r.user_id, u.name, u.phone, r.doctor_id, d.name, r.pet_id, "
                                             "COALESCE(p.pet_name, ''), COALESCE(r.reservation_type, ''), "
                                             "CAST(r.date AS CHAR), COALESCE(r.time_slot, ''), "
                                             "COALESCE(r.status, ''), CAST(r.created_at AS CHAR) "
                                             "FROM reaservations AS r "
                                             "LEFT JOIN users AS u ON r.user_id = u.id "
                                             "LEFT JOIN users AS d ON r.doctor_id = d.id "
                                             "LEFT JOIN pets AS p ON r.pet_id = p.id "
                                             "WHERE r.id = ? "
                                             "ORDER BY r.date DESC, r.created_at DESC")
                                       .bind(reservationId)
                                       .execute();

        if (result.count() == 0)
        {
            return nlohmann::json(); // 返回空JSON表示未找到
        }

        auto row = result.fetchOne();
        nlohmann::json reservation{
            {"id", row[0].get<int>()},
            {"user_id", row[1].get<int>()},
            {"user_name", row[2].isNull() ? "" : row[2].get<std::string>()},
            {"phone", row[3].isNull() ? "" : row[3].get<std::string>()},
            {"doctor_id", row[4].get<int>()},
            {"doctor_name", row[5].isNull() ? "" : row[5].get<std::string>()},
            {"pet_id", row[6].get<int>()},
            {"pet_name", row[7].isNull() ? "" : row[7].get<std::string>()},
            {"reservation_type", row[8].isNull() ? "" : row[8].get<std::string>()},
            {"date", row[9].isNull() ? "" : row[9].get<std::string>()},
            {"time_slot", row[10].isNull() ? "" : row[10].get<std::string>()},
            {"status", row[11].isNull() ? "预约成功" : row[11].get<std::string>()},
            {"created_at", row[12].isNull() ? "" : row[12].get<std::string>()}};

        return reservation;
    }
    catch (const std::exception &e)
    {
        return nlohmann::json(); // 返回空JSON表示错误
    }
}

//  获得预约记录接口
crow::response reservationCommonHandler::getReservationInformation(const crow::request &req, int &reservationId)
{
    try
    {
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取预约记录", "database connection failed", reservationId > 0 ? std::optional<int>(reservationId) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        nlohmann::json data = getReservationData(reservationId);
        if (data.empty())
        {
            return ResponseHelper::notFound(req);
        }

        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取预约记录", e.what(), reservationId > 0 ? std::optional<int>(reservationId) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to fetch reservations", e.what());
    }
}
