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

    crow::response getReservationSummary(const crow::request &req, int userId);            // 获取预约摘要，对应 /api/user/reservations 与 /api/doctor/reservations/summary

    nlohmann::json getReservationData(const int &reservationId);                           // 预约详情内部组装函数
    crow::response getReservationInformation(const crow::request& req, int &reservationId); // 获取预约详情，对应 /api/user/reservations/<int> 与 /api/doctor/reservations/reservationInformation/<int>
    
};

#endif
