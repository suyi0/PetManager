#ifndef PETCOMMONHANDLER_H
#define PETCOMMONHANDLER_H

#include "../../../utils/Utils.h"
#include "../../../services/logger/operationLogger.h"

class petCommonHandler : public BaseHandler {
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit petCommonHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response getPetProfiles(const crow::request& req, int userId);                  // 获取宠物档案，对应 /api/user/petProfiles 与 /api/doctor/userProfiles/<int>/petsProfile

    crow::response createPetProfile(const crow::request& req, int userId);                // 创建宠物档案，对应 /api/user/petProfiles 与 /api/doctor/userProfiles/<int>/petsProfile

    crow::response updatePetProfile(const crow::request& req, int userId, int petId);     // 更新宠物档案，对应 /api/user/petProfiles/<int> 与 /api/doctor/userProfiles/<int>/petsProfile/<int>

    crow::response deletePetProfile(const crow::request& req, int userId, int petId);     // 软删除宠物档案，对应 /api/user/petProfiles/<int> 与 /api/doctor/userProfiles/<int>/petsProfile/<int>
};

#endif
