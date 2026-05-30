#ifndef DOCTORHANDLER_H
#define DOCTORHANDLER_H 

#include "RoleTypeUtils/RoleTypeUtils.h"
#include "../../../utils/Utils.h"
#include "../../OperationLogger/OperationLogger.h"

class doctorHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit doctorHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response getDoctor(const crow::request& req);                         // 获取上班医生信息  对应 /api/doctor/getDoctor

    crow::response getDutyStatus(const crow::request& req, int userId);        // 获取当前医生签到状态 对应 /api/doctor/dutyStatus

    crow::response updateReservationStatus(const crow::request& req, int doctorId, int reservationId);    // 更新医生名下预约状态
    
    crow::response createOrderRecord(const crow::request& req, int doctorId);    // 创建诊单记录 对应 /api/doctor/orderRecord
    
    crow::response getUserList(const crow::request &req, const std::string data, const std::string &identifier);   // 获取用户信息 对应 /api/doctor/getUserList

    crow::response getUserProfiles(const crow::request& req, int userId);                 // 获取医生端用户详细档案 对应 /api/doctor/userProfiles/<int>

    crow::response handleDutyAction(const crow::request& req, int userId, bool requireDoctorId = false);   // 根据状态执行签到或签退

    crow::response onlineDoctor(const crow::request& req, int userId);      // 医生上班 对应 /api/doctor/onlineDoctor

    crow::response offlineDoctor(const crow::request& req, int userId);       // 医生下班 对应 /api/doctor/offlineDoctor

    nlohmann::json getOrderData(const int &orderId);
    crow::response changeOrder(const crow::request& req,int &orderId);             //修改订单 对应 /api/doctor/order/changeOrder/<int>

};


#endif
