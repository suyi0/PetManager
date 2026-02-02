#ifndef RESERVATION_HANDLER_H
#define RESERVATION_HANDLER_H

#include "crow.h"
#include "nlohmann/json.hpp"
#include "../../../database/DatabaseManagerInterface.h"
#include "./GetSchedule/GetSchedule.h"
#include "../../../utils/Utils.h"

class ReservationHandler {
private:
    DatabaseManagerInterface* dbManager;

public:
    explicit ReservationHandler(DatabaseManagerInterface* dbManager) : dbManager(dbManager) {}
    crow::response createReservation(const crow::request& req, int user_id, std::string name, std::string email, std::string phone, int doctor_id, std::string date, std::string time_slot, std::string status, std::string creation_time);                 //  对应 /api/reservate/record
    crow::response getReservations(const crow::request& req, int user_id);                   //  对应 /api/reservate/getrecord
    crow::response updateReservation(const crow::request& req, int id);
    nlohmann::json getReservationData();                                        // 对应 /api/reservate/getData
    crow::response getDoctorList(const crow::request& req);                     // 对应 /api/reservate/getDoctor
    crow::response cancelReservation(const crow::request& req, int user_id, int reservation_id);         // 对应 /api/reservate/cancel
    crow::response deleteReservation(const crow::request& req, int user_id, int reservation_id);         // 对应 /api/reservate/deleterecord
};

#endif