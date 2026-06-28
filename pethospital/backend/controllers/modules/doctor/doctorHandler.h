#ifndef DOCTORHANDLER_H
#define DOCTORHANDLER_H 

#include "roleTypeUtils/roleTypeUtils.h"
#include "../../../utils/Utils.h"
#include "../../../services/logger/operationLogger.h"

class doctorHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit doctorHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response getDoctor(const crow::request& req);                         // 获取上班医生信息  对应 /api/users/me/reservation-doctors

    crow::response getDutyStatus(const crow::request& req, int userId);        // 获取当前医生签到状态 对应 /api/doctors/duty-status

    crow::response updateReservationStatus(const crow::request& req, int doctorId, int reservationId);    // 更新医生名下预约状态 对应 /api/doctors/reservations/<int>/statuses
    
    crow::response createOrderRecord(const crow::request& req, int doctorId);    // 创建诊单记录 对应 /api/doctors/order-records
    
    crow::response getUserList(const crow::request &req, const std::string data, const std::string &identifier);   // 获取用户信息 对应 /api/doctors/user-summaries

    crow::response getUserProfiles(const crow::request& req, int userId);                 // 获取医生端用户详细档案 对应 /api/doctors/user-profiles/<int>

    crow::response getMedicineList(const crow::request& req);   // 获取药品列表 对应 /api/doctors/medicines

    crow::response searchMedicines(const crow::request& req, const std::string& keyword); // 搜索可开单药品 对应 /api/doctors/medicine-search

    crow::response handleDutyAction(const crow::request& req, int userId, bool requireDoctorId = false);   // 根据状态执行签到或签退 对应 /api/doctors/duty-status-changes

    crow::response onlineDoctor(const crow::request& req, int userId);      // 医生上班，由 /api/doctors/duty-status-changes 分发

    crow::response offlineDoctor(const crow::request& req, int userId);       // 医生下班，由 /api/doctors/duty-status-changes 分发

    nlohmann::json getOrderData(const int &orderId);                          // 订单详情内部组装函数
    crow::response changeOrder(const crow::request& req,int &orderId);             //修改订单 对应 /api/doctors/orders/<int>/changes

    nlohmann::json buildQueuesData(const int &doctorId);
    crow::response getQueues(const crow::request &req, const int doctorId);      // 获取待接诊队列 对应 /api/doctors/queues
};


#endif
