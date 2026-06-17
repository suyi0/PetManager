#include "searchCommonHandler.h"
#include "roleTypeUtils/roleTypeUtils.h"

namespace
{
    // 去除首尾空白
    std::string trimSearchText(std::string value)
    {
        // 判断是否为空
        const auto notSpace = [](unsigned char ch)
        {
            return !std::isspace(ch); // std::isspace()->判断字符是否为空白
        };

        // std::find_if(begin, end, 判断函数) - 从begin开始，找到第一个满足条件的元素，返回该元素的迭代器
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
        value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());

        return value;
    }
}

crow::response searchCommonHandler::searchDataUpdate(const crow::request &req, const int userId)
{
    try
    {
        if (userId <= 0)
        {
            return ResponseHelper::unauthorized(req, "用户身份无效");
        }

        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
        {
            return res;
        }
        auto &request_body = request_body_opt.value();

        const std::string searchText = trimSearchText(getRequestString(request_body, "searchText", ""));

        if (searchText.empty())
        {
            return ResponseHelper::validation(req, "搜索内容不能为空");
        }

        if (searchText.size() > 255)
        {
            return ResponseHelper::validation(req, "搜索内容不能超过255个字符");
        }

        dbManager->getSession()
            ->sql("INSERT INTO userSearch (user_id, search_text) "
                  "VALUES (?, ?) "
                  "ON DUPLICATE KEY UPDATE "
                  "updated_at = CURRENT_TIMESTAMP, "
                  "is_deleted = 0, "
                  "deleted_at = NULL, "
                  "deleted_by = NULL")
            .bind(userId, searchText)
            .execute();

        return ResponseHelper::success(req, {{"searchText", searchText}});
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "搜索", "更新搜索记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Internal Server Error", "服务器内部错误");
    }
}

nlohmann::json searchCommonHandler::getSearchHistory(const int userId)
{
    if (userId <= 0)
    {
        return nlohmann::json::array();
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                   ->sql("SELECT search_text "
                                         "FROM userSearch "
                                         "WHERE user_id = ? AND is_deleted = 0 "
                                         "ORDER BY updated_at DESC "
                                         "LIMIT 10;")
                                   .bind(userId)
                                   .execute();

    nlohmann::json searchHistory = nlohmann::json::array();
    for (const auto &row : result)
    {
        searchHistory.push_back(row[0].isNull() ? "" : row[0].get<std::string>());
    }
    return searchHistory;
}

crow::response searchCommonHandler::getSearchHistoryData(const crow::request &req, const int userId)
{
    try
    {
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "搜索", "获取搜索记录", "database connection failed", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        if (userId <= 0)
        {
            return ResponseHelper::unauthorized(req, "用户身份无效");
        }

        nlohmann::json data = getSearchHistory(userId);

        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "搜索", "获取搜索记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Internal Server Error", "服务器内部错误");
    }
}

crow::response searchCommonHandler::searchByKeyword(const crow::request &req, const int userId)
{
    try
    {
        if (userId <= 0)
        {
            return ResponseHelper::unauthorized(req, "用户身份无效");
        }

        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
        {
            return res;
        }
        auto &request_body = request_body_opt.value();

        const std::string searchType = getRequestString(request_body, "searchType", "");
        const std::string searchByKeyword = trimSearchText(getRequestString(request_body, "searchByKeyword", ""));

        if (searchByKeyword.empty())
        {
            return ResponseHelper::validation(req, "搜索内容不能为空");
        }
        if (searchByKeyword.size() > 255)
        {
            return ResponseHelper::validation(req, "搜索内容不能超过255个字符");
        }
        if (searchType.empty())
        {
            return ResponseHelper::validation(req, "搜索类型不能为空");
        }

        const std::string roleName = RoleTypeUtils::getUserRoleName(dbManager, userId);
        const bool isBoss = RoleTypeUtils::isBossRole(roleName);
        const bool isMedicalStaff = RoleTypeUtils::isMedicalStaffRole(roleName);

        const std::string keywordLike = "%" + searchByKeyword + "%";
        std::string sql = "";
        if (searchType == "orders")
        {
            const std::string filterSql = isBoss
                                              ? "WHERE o.is_deleted = 0 "
                                          : isMedicalStaff ? "WHERE o.doctor_id = ? AND o.is_deleted = 0 "
                                                           : "WHERE o.owner_id = ? AND o.is_deleted = 0 ";

            sql = "SELECT o.id, p.pet_name, COALESCE(d.name, ''), o.order_type, "
                  "COALESCE(o.order_data, ''), COALESCE(o.order_status, '待付款'), COALESCE(o.order_totalprice, 0.0) "
                  "FROM orders AS o "
                  "LEFT JOIN pets AS p ON o.pet_id = p.id "
                  "LEFT JOIN users AS d ON o.doctor_id = d.id " +
                  filterSql +
                  "AND (COALESCE(p.pet_name, '') LIKE ? "
                  "OR COALESCE(d.name, '') LIKE ? "
                  "OR COALESCE(o.order_type, '') LIKE ? "
                  "OR COALESCE(o.order_data, '') LIKE ? "
                  "OR COALESCE(o.order_status, '') LIKE ?) "
                  "ORDER BY o.updated_at DESC, o.id DESC "
                  "LIMIT 20";
        }
        else if (searchType == "reservations")
        {
            const std::string filterSql = isBoss
                                              ? "WHERE r.is_deleted = 0 "
                                          : isMedicalStaff ? "WHERE r.doctor_id = ? AND r.is_deleted = 0 "
                                                           : "WHERE r.user_id = ? AND r.is_deleted = 0 ";

            sql = "SELECT r.id, COALESCE(p.pet_name, ''), COALESCE(d.name, ''), "
                  "CAST(r.date AS CHAR), COALESCE(r.time_slot, ''), "
                  "COALESCE(r.reservation_type, ''), COALESCE(r.status, '') "
                  "FROM reservations AS r "
                  "LEFT JOIN pets AS p ON r.pet_id = p.id "
                  "LEFT JOIN users AS d ON r.doctor_id = d.id " +
                  filterSql +
                  "AND (COALESCE(p.pet_name, '') LIKE ? "
                  "OR COALESCE(d.name, '') LIKE ? "
                  "OR CAST(r.date AS CHAR) LIKE ? "
                  "OR COALESCE(r.time_slot, '') LIKE ? "
                  "OR COALESCE(r.reservation_type, '') LIKE ? "
                  "OR COALESCE(r.status, '') LIKE ?) "
                  "ORDER BY r.updated_at DESC, r.id DESC "
                  "LIMIT 20";
        }
        else
        {
            return ResponseHelper::validation(req, "搜索类型不支持");
        }

        auto query = dbManager->getSession()->sql(sql);
        if ((searchType == "orders" || searchType == "reservations") && !isBoss)
        {
            query.bind(userId);
        }

        if (searchType == "orders")
        {
            query.bind(keywordLike, keywordLike, keywordLike, keywordLike, keywordLike);
        }
        else if (searchType == "reservations")
        {
            query.bind(keywordLike, keywordLike, keywordLike, keywordLike, keywordLike, keywordLike);
        }
        mysqlx::SqlResult result = query.execute();

        nlohmann::json data = nlohmann::json::array();
        if (searchType == "orders")
        {
            for (auto row : result)
            {
                nlohmann::json searchResult;
                searchResult["id"] = row[0].get<int>();
                searchResult["pet_name"] = row[1].isNull() ? "" : row[1].get<std::string>();
                searchResult["doctor_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
                searchResult["order_type"] = row[3].isNull() ? "" : row[3].get<std::string>();
                searchResult["order_data"] = row[4].isNull() ? "" : row[4].get<std::string>();
                searchResult["order_status"] = row[5].isNull() ? "待付款" : row[5].get<std::string>();
                searchResult["order_totalprice"] = row[6].isNull() ? 0.0 : row[6].get<double>();
                data.push_back(searchResult);
            }
        }
        else if (searchType == "reservations")
        {
            for (auto row : result)
            {
                nlohmann::json searchResult;
                searchResult["id"] = row[0].get<int>();
                searchResult["pet_name"] = row[1].isNull() ? "" : row[1].get<std::string>();
                searchResult["doctor_name"] = row[2].isNull() ? "" : row[2].get<std::string>();
                searchResult["date"] = row[3].isNull() ? "" : row[3].get<std::string>();
                searchResult["time_slot"] = row[4].isNull() ? "" : row[4].get<std::string>();
                searchResult["reservation_type"] = row[5].isNull() ? "" : row[5].get<std::string>();
                searchResult["status"] = row[6].isNull() ? "预约成功" : row[6].get<std::string>();
                data.push_back(searchResult);
            }
        }
        return ResponseHelper::success(req, data);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "搜索", "搜索", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Internal Server Error", "服务器内部错误");
    }
}
