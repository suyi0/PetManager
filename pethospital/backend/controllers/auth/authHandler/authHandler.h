#ifndef AUTHHANDLER_H
#define AUTHHANDLER_H 


#include "../../../utils/Utils.h"
#include "../../auth/Verification/Verification.h"
#include "../../auth/Encrypt/Encrypt.h"
#include "../JwtUtils/JwtUtils.h"

// 添加邮箱格式验证函数
bool isValidEmailFormat(const std::string &email);

// 添加手机格式验证函数
bool isValidPhoneFormat(const std::string &phone);

// 添加用户Token验证函数
int isValidUserToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);
int isValidUserorderToken(const crow::request &req, crow::response &res, int &orderId, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 添加超级管理员Token验证函数
int isValidSuperAdminToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

class authHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:
    explicit authHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response authCheckName(const crow::request& req);                 //  对应 "/api/auth/checkName"

    crow::response authCheckEmail(const crow::request& req);                // 对应 "/api/auth/checkEmail"

    crow::response authCheckPhone(const crow::request& req);                //  对应 "/api/auth/checkPhone"

    crow::response authReadyVerification(const crow::request& req);         //  对应 "/api/verification/ready"

    crow::response authVerification(const crow::request& req);              //  对应 "/api/auth/verify"

    // 新增：手机号验证码相关函数
    crow::response sendSmsVerification(const crow::request& req);           // 对应 "/api/verification/sms/send"
    crow::response checkVerifySmsCode(const crow::request& req);                 // 对应 "/api/verification/sms/verify"
};

#endif
