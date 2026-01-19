
#ifndef USERHANDLER_H
#define USERHANDLER_H 

#include <mysqlx/xdevapi.h>
#include "../../../../src/DatabaseManager/DatabaseManager.h"
#include "../../../Shared/Utils/Utils.h"
#include "nlohmann/json.hpp"
#include "../../Auth/Verification/Verification.h"
#include "../../Auth/UserEncrypt/Encrypt.h"
#include "../User/User.h"
#include "../GetAddress/GetAddress.h"

class UserHandler {
private:
    DatabaseManagerInterface* dbManager;
public:
    explicit UserHandler(DatabaseManagerInterface* dbManager) : dbManager(dbManager) {}

    crow::response userLogin(const crow::request& req); //  对应 "/api/user/login"

    crow::response userReadyVerification(const crow::request& req); //  对应 "/api/verification/ready"

    crow::response userVerification(const crow::request& req); //   对应 "/api/verification/verify"

    crow::response userUpdate(const crow::request& req); // 对应 "/api/user/form"

    crow::response userUploadAvatar(const crow::request& req); // 对应 "/api/user/upload/avatar"

    crow::response upload(const crow::request& req, const std::string& filename); // 对应 "/uploads/<string>"

    crow::response getUserdata(const crow::request& req); // 对应 "/api/user/data"

};

#endif