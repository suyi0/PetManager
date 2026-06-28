#ifndef AUTHHANDLER_H
#define AUTHHANDLER_H 


#include "../../../utils/Utils.h"
#include "../../../utils/AuthIdentifierUtils.h"
#include "../../../services/verification/verification.h"
#include "roleTypeUtils/roleTypeUtils.h"


class authHandler : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:
    explicit authHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response authCheckName(const crow::request& req);                 //  对应 "/api/auth/checkName"

    crow::response authCheckEmail(const crow::request& req);                // 对应 "/api/auth/checkEmail"

    crow::response authCheckPhone(const crow::request& req);                //  对应 "/api/auth/checkPhone"

    crow::response getEmailVerification(const crow::request& req, int userId = -1);         //  对应 "/api/verification/email/register"、"/api/verification/email/change"

    crow::response checkVerifyEmailCode(const crow::request& req, int userId = -1);              //  对应 "/api/verification/email/verify/register"、"/api/verification/email/verify/change"

    crow::response refreshAdminToken(const crow::request& req);             //  对应 "/api/auth/admin/refresh"

    crow::response logout(const crow::request& req);                       //  对应 "/api/auth/logout"（管理端登出服务端吊销）

    // 新增：手机号验证码相关函数
    crow::response sendSmsVerification(const crow::request& req);           // 对应 "/api/verification/sms/send"
    crow::response checkVerifySmsCode(const crow::request& req, int userId = -1);                 // 对应 "/api/verification/sms/verify"
};

#endif
