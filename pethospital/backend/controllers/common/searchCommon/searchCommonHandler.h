#ifndef SEARCHCOMMONHANDLER_H
#define SEARCHCOMMONHANDLER_H

#include "../../../utils/Utils.h"
#include "../../../services/logger/operationLogger.h"

class searchCommonHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit searchCommonHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response searchDataUpdate(const crow::request& req, const int userId);

    nlohmann::json getSearchHistory(const int userId);
    crow::response getSearchHistoryData(const crow::request &req, const int userId);

    crow::response searchByKeyword(const crow::request &req, const int userId);
};

#endif