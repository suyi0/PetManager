#ifndef AUTHHANDLER_H
#define AUTHHANDLER_H 


#include "../../../utils/Utils.h"
#include "../../../utils/AuthIdentifierUtils.h"
#include "../../auth/Verification/Verification.h"
#include "../../auth/Encrypt/Encrypt.h"
#include "../JwtUtils/JwtUtils.h"

// 添加用户Token验证函数
int isValidUserToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);
int isValidUserorderToken(const crow::request &req, crow::response &res, int &orderId, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 验证管理端 token，允许管理类角色访问管理相关接口
int isValidManagementToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 验证人事门户角色 token，仅允许“人事经理”访问人事相关接口
int isValidPersonnelToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 验证医疗端 token，允许医生、护士访问医疗端接口
int isValidMedicalStaffToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 验证仓储端 token，允许仓储类角色访问仓库接口
int isValidWarehouseStaffToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

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

    crow::response checkVerifyEmailCode(const crow::request& req);              //  对应 "/api/verification/email/verify"

    crow::response refreshAdminToken(const crow::request& req);             //  对应 "/api/auth/admin/refresh"

    // 新增：手机号验证码相关函数
    crow::response sendSmsVerification(const crow::request& req);           // 对应 "/api/verification/sms/send"
    crow::response checkVerifySmsCode(const crow::request& req);                 // 对应 "/api/verification/sms/verify"
};

#endif
