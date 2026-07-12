#ifndef AUTHMIDDLEWARE_H
#define AUTHMIDDLEWARE_H

#include "../../utils/Utils.h"
#include "../../controllers/auth/jwtUtils/jwtUtils.h"
#include <string>

// 用户端 token 校验。
int isValidUserToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 任意已登录账号 token 校验，仅用于修改本人密码等账号级自助操作。
int isValidAuthenticatedToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 用户订单资源 token 校验。
int isValidUserorderToken(const crow::request &req, crow::response &res, int &orderId, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 管理端 token 校验。
int isValidManagementToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 指定功能权限 token 校验。
int isValidPermissionToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &permissionKey);

// 超级管理员门户 token 校验。
int isValidSuperAdminPortalToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 财务门户 token 校验。
int isValidFinancePortalToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 总裁门户 token 校验。
int isValidBossPortalToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 人事端 token 校验。
int isValidPersonnelToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 医疗端 token 校验。
int isValidMedicalStaffToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

// 仓储端 token 校验。
int isValidWarehouseStaffToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager);

#endif
