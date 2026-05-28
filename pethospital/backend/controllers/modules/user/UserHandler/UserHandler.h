
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

    crow::response userLogin(const crow::request& req); //  对应 登录 "/api/user/login"
    
    crow::response userUpdate(const crow::request& req, int authenticatedUserId = -1); // 对应 更新信息 "/api/user/form"
    
    crow::response userUploadAvatar(const crow::request& req); // 对应 上传头像 "/api/user/upload/avatar"

    crow::response upload(const crow::request& req, const std::string& filename); // 对应 上传 "/uploads/<string>"

    crow::response getPetProfiles(const crow::request& req, int userId);

    crow::response createPetProfile(const crow::request& req, int userId);

    crow::response updatePetProfile(const crow::request& req, int userId, int petId);

    crow::response deletePetProfile(const crow::request& req, int userId, int petId);

    nlohmann::json getUserData(const int &id);  // 对应 获取用户数据

    crow::response createReservation(const crow::request& req, int user_id, int pet_id, int doctor_id, std::string reservation_type, std::string date, std::string time_slot, std::string status);                 //  对应 /api/user/reservate/record

    nlohmann::json getReservationDate();                                        // 对应 /api/user/reservate/getDate

    crow::response getDoctorList(const crow::request& req);                     // 对应 /api/user/reservate/getDoctor

    crow::response cancelReservation(const crow::request& req, int userId, int reservationId);         // 对应 /api/user/reservate/cancel

    crow::response deleteReservation(const crow::request& req, int userId, int reservationId);         // 对应 /api/user/reservate/deleterecord

};

#endif
