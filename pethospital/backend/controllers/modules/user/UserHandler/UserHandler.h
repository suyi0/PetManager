
#ifndef USERHANDLER_H
#define USERHANDLER_H 

#include <mysqlx/xdevapi.h>
#include <unordered_map>
#include <chrono>
#include "../../../../database/DatabaseManager.h"
#include "../../../../utils/Utils.h"
#include "../../../auth/Encrypt/Encrypt.h"
#include "../../../../models/user/User.h"
#include "../GetAddress/GetAddress.h"
#include "../../../auth/JwtUtils/JwtUtils.h"

class UserHandler : public BaseHandler {
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit UserHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}
    // 添加一个方法来检查数据库管理器是否可用
    bool isDbManagerValid() const { return dbManager != nullptr; }

    crow::response userLogin(const crow::request& req); //  对应 "/api/user/login"
    
    crow::response userUpdate(const crow::request& req); // 对应 "/api/user/form"
    
    crow::response userUploadAvatar(const crow::request& req); // 对应 "/api/user/upload/avatar"

    crow::response upload(const crow::request& req, const std::string& filename); // 对应 "/uploads/<string>"

    nlohmann::json getUserData(const int &id);

};

#endif
