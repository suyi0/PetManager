#ifndef RESERVATIONCOMMONHANDLER_H
#define RESERVATIONCOMMONHANDLER_H

#include "../../../utils/Utils.h"
#include "../../OperationLogger/OperationLogger.h"

class reservationCommonHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit reservationCommonHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response getReservationSummary(const crow::request &req, int userId);            // 获取预约列表

    nlohmann::json getReservationData(const int &reservationId);
    crow::response getReservationInformation(const crow::request& req, int &reservationId);        // 获取预约记录
    
};

#endif
