
#ifndef USERHANDLER_H
#define USERHANDLER_H 

#include <mysqlx/xdevapi.h>
#include <unordered_map>
#include <chrono>
#include "../GetSchedule/GetSchedule.h"
#include "../../../../database/DatabaseManager.h"
#include "../../../../utils/Utils.h"
#include "../../../auth/Encrypt/Encrypt.h"
#include "../../../../models/user/User.h"
#include "../GetAddress/GetAddress.h"
#include "../../../auth/JwtUtils/JwtUtils.h"
#include "RoleTypeUtils/RoleTypeUtils.h"
#include "../../../OperationLogger/OperationLogger.h"

class userHandler : public BaseHandler {
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit userHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}
    // 添加一个方法来检查数据库管理器是否可用
    bool isDbManagerValid() const { return dbManager != nullptr; }

    crow::response userUpdate(const crow::request& req, int userId = -1); // 对应注册 "/api/user/register" 与更新资料 "/api/user/profile"
    
    crow::response userLogin(const crow::request& req); //  对应 登录 "/api/user/login"
    
    crow::response addNewAddress(const crow::request &req, int userId); // 添加新地址 "/api/user/address"

    crow::response addressUpdate(const crow::request &req, int userId, int addressId); // 对应地址更新 "/api/user/address/<int>"

    crow::response userUploadAvatar(const crow::request& req); // 对应 上传头像 "/api/user/avatar"

    crow::response upload(const crow::request& req, const std::string& filename); // 对应 上传 "/uploads/<string>"

    nlohmann::json getUserData(const int &id);  // 获取用户基础数据内部组装函数

    crow::response createReservation(const crow::request& req, int user_id, int pet_id, int doctor_id, std::string reservation_type, std::string date, std::string time_slot, std::string status);                 // 对应 /api/user/reservations

    nlohmann::json getReservationDate();                                        // 对应 /api/user/reservations/dates

    crow::response getDoctorList(const crow::request& req);                     // 对应 /api/user/reservations/doctors

    crow::response cancelReservation(const crow::request& req, int userId, int reservationId);         // 对应 /api/user/reservations/<id>/cancel

    crow::response deleteReservation(const crow::request& req, int userId, int reservationId);         // 对应 DELETE /api/user/reservations/<int>

};

#endif
