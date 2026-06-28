
#ifndef USERHANDLER_H
#define USERHANDLER_H

#include <mysqlx/xdevapi.h>
#include <unordered_map>
#include <chrono>
#include "../getSchedule/getSchedule.h"
#include "../../../../database/DatabaseManager.h"
#include "../../../../utils/Utils.h"
#include "../../../auth/encrypt/encrypt.h"
#include "../../../../models/user/User.h"
#include "../getAddress/getAddress.h"
#include "../../../auth/jwtUtils/jwtUtils.h"
#include "roleTypeUtils/roleTypeUtils.h"
#include "../../../../services/logger/operationLogger.h"

class userHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit userHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}
    // 添加一个方法来检查数据库管理器是否可用
    bool isDbManagerValid() const { return dbManager != nullptr; }

    crow::response userUpdate(const crow::request &req, int userId = -1); // 对应注册 "/api/users/registrations" 与更新资料 "/api/users/me/profile"

    crow::response updatePassword(const crow::request &req, int userId);    // 对应 修改密码 "/api/users/me/password"

    crow::response updateEmail(const crow::request &req, int userId);   // 对应 修改邮箱 "/api/users/me/email"

    crow::response updatePhone(const crow::request &req, int userId);   // 对应 修改手机 "/api/users/me/phone"

    crow::response userLogin(const crow::request &req); //  对应 登录 "/api/users/sessions"

    crow::response addNewAddress(const crow::request &req, int userId); // 添加新地址 "/api/users/me/addresses"

    crow::response addressUpdate(const crow::request &req, int userId, int addressId); // 对应地址更新 "/api/users/me/addresses/<int>"

    crow::response userUploadAvatar(const crow::request &req); // 对应 上传头像 "/api/users/me/avatar"

    crow::response upload(const crow::request &req, const std::string &filename); // 对应 上传 "/uploads/<string>"

    nlohmann::json getUserData(const int &id); // 获取用户基础数据内部组装函数

    crow::response createReservation(const crow::request &req, int user_id, int pet_id, int doctor_id, std::string reservation_type, std::string date, std::string time_slot, std::string status); // 对应 /api/users/me/reservations

    nlohmann::json getReservationDate(); // 对应 /api/users/me/reservation-dates

    crow::response getDoctorList(const crow::request &req); // 对应 /api/users/me/reservation-doctors

    crow::response cancelReservation(const crow::request &req, int userId, int reservationId); // 对应 /api/users/me/reservations/<id>/cancellation

    crow::response deleteReservation(const crow::request &req, int userId, int reservationId); // 对应 /api/users/me/reservations/<int>

    crow::response toTheHospital(const crow::request &req, int userId, int reservationId); // 对应 /api/users/me/reservations/<int>/to-the-hospital
};

#endif
