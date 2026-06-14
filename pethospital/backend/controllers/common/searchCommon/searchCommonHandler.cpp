#include "searchCommonHandler.h"


crow::response searchCommonHandler::searchDataUpdate(const crow::request& req, const int userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
        {
            return res;
        }
        auto &body = request_body_opt.value();

        const std::string searchText = 
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
        searchHistory.push_back(row[0].get<std::string>());
    }
    return searchHistory;
}

crow::response searchCommonHandler::getSearchData(const crow::request &req, const int userId)
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
