#ifndef PETCOMMONHANDLER_H
#define PETCOMMONHANDLER_H

#include "../../../utils/Utils.h"
#include "../../OperationLogger/OperationLogger.h"

class petCommonHandler : public BaseHandler {
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit petCommonHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response getPetProfiles(const crow::request& req, int userId);

    crow::response createPetProfile(const crow::request& req, int userId);

    crow::response updatePetProfile(const crow::request& req, int userId, int petId);

    crow::response deletePetProfile(const crow::request& req, int userId, int petId);
};

#endif