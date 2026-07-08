#include "userHandler.h"
#include "../../../../utils/AuthIdentifierUtils.h"
#include "../userPhoneSync/userPhoneSync.h"
#include "../../../../services/auth/AuthSessionStore.h"
#include "../../../../services/auth/AuthLoginFailureStore.h"
#include "../../../../services/rbac/RbacService.h"
#include "../../../../services/redis/RedisClient.h"
#include "../../../../services/redis/redisLock/RedisLock.h"
#include "../../../../services/redis/doctorListCache/DoctorListCache.h"
#include "../../../../utils/requestUtils/RequestUtils.h"
#include "../../../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../../../services/realtime/doctorBroadcaster/doctorQueueBroadcaster.h"
#include "../../../../services/realtime/doctorListBroadcaster/doctorListBroadcaster.h"
#include "statusLabelUtils/StatusLabelUtils.h"
#include <vector>
#include "userHandlerInternal.h"

// userHandler 登录域：账号密码登录（含登录失败锁定、会话版本、令牌签发）。
// 从 userHandler.cpp 按域拆出；类声明仍在 userHandler.h。

crow::response userHandler::userLogin(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        // 检查必要字段是否存在
        bool hasIdentifier = (request_body.find("identifier") != request_body.end());
        bool hasEmail = (request_body.find("email") != request_body.end());
        bool hasPhone = (request_body.find("phone") != request_body.end());
        bool hasPassword = (request_body.find("password") != request_body.end());

        if (!hasPassword || (!hasIdentifier && !hasEmail && !hasPhone))
        {
            return ResponseHelper::error(req, "Missing identifier and password");
        }

        std::string identifier = "";
        std::string email = "";
        std::string phone = "";
        std::string password = "";

        // 安全地获取字段值
        if (hasIdentifier)
        {
            identifier = request_body["identifier"].is_string() ? request_body["identifier"].get<std::string>() : request_body["identifier"].dump();
            identifier = clean_string(identifier);
        }
        if (hasEmail)
        {
            email = request_body["email"].is_string() ? request_body["email"].get<std::string>() : request_body["email"].dump();
            email = clean_string(email);
        }
        if (hasPhone)
        {
            phone = request_body["phone"].is_string() ? request_body["phone"].get<std::string>() : request_body["phone"].dump();
            phone = normalizePhoneIdentifier(phone);
        }
        if (hasPassword)
        {
            password = request_body["password"].is_string() ? request_body["password"].get<std::string>() : request_body["password"].dump();
        }

        const int identifierFieldCount =
            (!identifier.empty() ? 1 : 0) +
            (!email.empty() ? 1 : 0) +
            (!phone.empty() ? 1 : 0);

        if (identifierFieldCount == 0)
        {
            return ResponseHelper::validation(req, "Identifier is required");
        }

        if (identifierFieldCount > 1)
        {
            return ResponseHelper::validation(req, "Provide only one of identifier, email, or phone");
        }

        if (!identifier.empty())
        {
            if (isValidEmailFormat(identifier))
            {
                email = identifier;
            }
            else if (isValidPhoneFormat(identifier))
            {
                phone = normalizePhoneIdentifier(identifier);
            }
            else
            {
                return ResponseHelper::validation(req, "Identifier must be a valid email or phone");
            }
        }

        // 登录失败锁定：用「客户端 IP + 标识符」组合键。避免攻击者拿受害者标识符把对方账号锁死（DoS）——
        // 只有同一 IP 对同一标识符反复失败才会被锁，受害者从自己 IP 登录不受影响。
        // 多实例共享；Redis 不可用则整段跳过，绝不阻断登录。
        const std::string loginIdentifier = !email.empty() ? email : phone;
        const std::string loginClientIp = RequestUtils::getClientIp(req);
        if (AuthLoginFailureStore::isLocked(loginClientIp, loginIdentifier))
        {
            return ResponseHelper::error(req, "尝试次数过多，请稍后再试");
        }

        // 手机号登录需要把手机号前缀去掉
        const auto stripChinaCountryCode = [](const std::string &value)
        {
            return value.rfind("+86", 0) == 0 ? value.substr(3) : value;
        };

        // 从数据库中获取用户信息
        // user是一个智能指针
        std::unique_ptr<User> user = nullptr;
        std::string role_name;
        try
        {
            // 获取表
            mysqlx::SqlResult result;
            if (!email.empty())
            {
                // 通过email查询用户
                result = dbManager->getSession()
                             ->sql("SELECT u.id, COALESCE(u.position_id, 0), "
                                   "CASE WHEN u.account_type = 'customer' THEN '普通用户' ELSE COALESCE(pos.name, '') END AS type_name, "
                                   "u.name, u.password, p.phone, u.email, "
                                   "CAST(u.birthday AS CHAR), u.head_image "
                                   "FROM users AS u "
                                   "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                   "LEFT JOIN phones AS p ON p.user_id = u.id "
                                   "WHERE u.email = ?")
                             .bind(email)
                             .execute();
            }
            else if (!phone.empty())
            {
                const std::string legacyPhone = stripChinaCountryCode(phone);
                // 通过phone查询用户
                result = dbManager->getSession()
                             ->sql("SELECT u.id, COALESCE(u.position_id, 0), "
                                   "CASE WHEN u.account_type = 'customer' THEN '普通用户' ELSE COALESCE(pos.name, '') END AS type_name, "
                                   "u.name, u.password, p.phone, u.email, "
                                   "CAST(u.birthday AS CHAR), u.head_image "
                                   "FROM users AS u "
                                   "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                   "JOIN phones AS p ON p.user_id = u.id "
                                   "WHERE p.phone = ? OR p.phone = ?")
                             .bind(phone)
                             .bind(legacyPhone)
                             .execute();
            }
            else
            {
                // 理论上不会到达这里，因为前面已经检查过了
                return ResponseHelper::system_error(req, "Either email or phone must be provided");
            }

            // 即使email变量包含恶意代码，也会被当作普通字符串值处理
            // 处理结果
            for (auto row : result)
            {
                user = std::make_unique<User>();
                user->setID(row[0].get<int>());
                // 确保正确处理所有字段，添加错误检查
                try
                {
                    user->setTypeID(row[1].get<int>());
                }
                catch (...)
                {
                    user->setTypeID(0);
                }
                try
                {
                    user->setName(clean_string(row[3].get<std::string>()));
                }
                catch (...)
                {
                    user->setName("");
                }
                try
                {
                    user->setPassword(row[4].get<std::string>());
                }
                catch (...)
                {
                    user->setPassword("");
                }
                try
                {
                    user->setPhone(row[5].get<std::string>());
                }
                catch (...)
                {
                    user->setPhone("");
                }
                try
                {
                    user->setEmail(row[6].get<std::string>());
                }
                catch (...)
                {
                    user->setEmail("");
                }
                // 处理生日字段，确保其格式正确
                try
                {
                    auto birthday_value = row[7];
                    if (birthday_value.isNull())
                    {
                        user->setBirthday(boost::gregorian::date(1970, 1, 1));
                    }
                    else
                    {
                        std::string birthday_str = birthday_value.get<std::string>();

                        // 确保格式为 YYYY-MM-DD
                        if (birthday_str.length() == 10 && birthday_str[4] == '-' && birthday_str[7] == '-')
                        {
                            try
                            {
                                user->setBirthday(boost::gregorian::from_simple_string(birthday_str));
                            }
                            catch (...)
                            {
                                user->setBirthday(boost::gregorian::date(1970, 1, 1));
                            }
                        }
                        else
                        {
                            std::cout << "Debug: Invalid birthday format: " << birthday_str << ", using default" << std::endl;
                            user->setBirthday(boost::gregorian::date(1970, 1, 1));
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    std::cout << "Debug: Exception in birthday processing: " << e.what() << std::endl;
                    user->setBirthday(boost::gregorian::date(1970, 1, 1));
                }
                try
                {
                    user->setHeadImage(clean_string(row[8].get<std::string>()));
                }
                catch (...)
                {
                    user->setHeadImage("");
                }
                try
                {
                    role_name = row[2].isNull() ? "" : row[2].get<std::string>();
                }
                catch (...)
                {
                    role_name.clear();
                }

                break; // 只需要第一个匹配的用户
            }
        }
        catch (const mysqlx::Error &e)
        {
            // 数据库操作错误
            std::cerr << "Database error: " << e.what() << std::endl;

            return ResponseHelper::database_error(req, "Database operation failed", e.what());
        }
        catch (const std::exception &e)
        {
            // 其他错误
            std::cerr << "Error: " << e.what() << std::endl;

            return ResponseHelper::operation_failed(req, "Operation failed", e.what());
        }

        // 在这里验证用户名和密码
        nlohmann::json response;
        if (!user || !verify_password_hash(password, user->getPassword()))
        {
            // 失败计数 + 达阈值锁定（对不存在的标识符同样计数，避免账号枚举）。
            AuthLoginFailureStore::recordFailure(loginClientIp, loginIdentifier);
            // 不区分用户不存在和密码错误，统一返回相同错误信息
            return ResponseHelper::error(req, "Invalid username or password");
        }
        else
        {
            // 登录成功：清掉失败计数与锁定。
            AuthLoginFailureStore::clearOnSuccess(loginClientIp, loginIdentifier);
            if (password_hash_needs_upgrade(user->getPassword()))
            {
                try
                {
                    dbManager->getSession()
                        ->sql("UPDATE users SET password = ? WHERE id = ?")
                        .bind(hash_password(password), user->getID())
                        .execute();
                }
                catch (const std::exception &e)
                {
                    OperationLogger::LogExceptionOperation(dbManager, req, "用户", "更新密码", "Failed to upgrade password hash for user " + std::to_string(user->getID()) + ": " + std::string(e.what()));
                }
            }

            // 验证成功
            // 生成一个基于用户邮箱的JWT token
            const bool loggedInWithEmail = !email.empty();
            const std::string loginIdentifier = loggedInWithEmail ? user->getEmail() : user->getPhone();
            if (role_name.empty())
            {
                role_name = "普通用户";
            }
            std::string token = JwtUtils::createToken(
                user->getID(),
                user->getTypeID(),
                role_name,
                loginIdentifier,
                loggedInWithEmail,
                RbacService::userHasManagementAccess(dbManager, user->getID()));
            response["token"] = token;
            response["success"] = true;

            // 手动构建用户JSON对象，确保birthday正确序列化
            nlohmann::json user_json;
            user_json["id"] = user->getID();
            user_json["type_id"] = user->getTypeID();
            user_json["type_name"] = role_name;
            user_json["name"] = user->getName();
            appendUserNameParts(user_json, user->getName());
            user_json["email"] = user->getEmail();
            user_json["phone"] = user->getPhone();
            user_json["head_image"] = user->getHeadImage();

            // 特别处理birthday字段，将其转换为字符串格式
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(4) << user->getBirthday().year()
                << "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(user->getBirthday().month())
                << "-" << std::setfill('0') << std::setw(2) << user->getBirthday().day();
            user_json["birthday"] = oss.str();

            response["user"] = user_json;

            // 登录即下发权限快照：前端在保存会话（选 storage、算首页）时就需要，
            // 不能等 /auth/me——那一步在 setSession 之后（判权数据源=permissions，名字仅展示）
            if (auto access = RbacService::loadUserAccess(dbManager, user->getID()))
            {
                nlohmann::json accessJson;
                accessJson["account_type"] = access->accountType;
                accessJson["position_id"] = access->positionId == 0 ? nlohmann::json(nullptr) : nlohmann::json(access->positionId);
                accessJson["position_name"] = access->positionName;
                accessJson["staff_kind"] = access->staffKind;
                accessJson["permissions"] = RbacService::effectivePermissions(*access);
                response["access"] = accessJson;
            }
            // Return response
            return ResponseHelper::success(req, response);
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "用户", "登录", "Exception occurred: " + std::string(e.what()));
        return ResponseHelper::system_error(req, "Internal server error" + std::string(e.what()));
    }
}
