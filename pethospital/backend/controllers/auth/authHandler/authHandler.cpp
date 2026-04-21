#include "authHandler.h"
#include "RoleTypeUtils/RoleTypeUtils.h"

namespace
{
    enum class TokenValidationScope
    {
        User,
        SuperAdmin
    };

    // 统一的Token验证函数，根据不同的权限范围进行验证
    int validateTokenWithScope(const crow::request &req,
                               crow::response &res,
                               std::shared_ptr<DatabaseManagerInterface> dbManager,
                               TokenValidationScope scope)
    {
        // 1. 从请求头中提取Token
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
        {
            res = ResponseHelper::unauthorized(req, "Missing or invalid token");
            return -1;
        }

        std::string token = authHeader.substr(7);
        if (token.empty())
        {
            res = ResponseHelper::unauthorized(req, "Empty token provided");
            return -1;
        }

        // 2. 解析Token获取用户信息
        auto claims = JwtUtils::getTokenClaims(token);
        if (!claims || claims->userId <= 0 || claims->identifier.empty())
        {
            res = ResponseHelper::unauthorized(req, "Invalid or expired token");
            return -1;
        }

        // 3. 验证数据库连接
        if (!dbManager || !dbManager->getSession() || !dbManager->getSchema())
        {
            res = ResponseHelper::system_error(req, "Database connection unavailable");
            return -1;
        }

        // 4. 根据权限范围验证用户是否有权限进行相应的操作
        std::string identifier = claims->identifier;
        bool isAuthorized = scope == TokenValidationScope::SuperAdmin
                                ? JwtUtils::isUserAuthorizedForAdminForm(claims->userId, identifier, claims->isEmailLogin, dbManager)
                                : JwtUtils::isUserAuthorizedForUserForm(claims->userId, identifier, claims->isEmailLogin, dbManager);

        // 5. 如果用户没有权限，返回未授权响应
        if (!isAuthorized)
        {
            res = ResponseHelper::unauthorized(req, "用户无权限进行此操作");
            return -1;
        }

        return claims->userId;
    }
}

// 验证用户token
int isValidUserToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    return validateTokenWithScope(req, res, dbManager, TokenValidationScope::User);
}

// 验证用户的订单token
int isValidUserorderToken(const crow::request &req, crow::response &res, int &orderId, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    // 1. 验证JWT token
    std::string authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
    {
        res = ResponseHelper::unauthorized(req, "Missing or invalid token");
        return -1;
    }

    std::string token = authHeader.substr(7); // 移除 "Bearer " 前缀

    // 2. 基本token验证
    if (token.empty()) {
        res = ResponseHelper::unauthorized(req, "Empty token provided");
        return -1;
    }

    // 3. 解析token获取用户信息
    int userId = JwtUtils::getUserIdFromToken(token);
    if (userId <= 0)
    {
        std::string errorMsg;
        switch (userId)
        {
        case -1:
            errorMsg = "Invalid token format";
            break;
        case -2:
            errorMsg = "Token signature invalid";
            break;
        case -3:
            errorMsg = "Token expired";
            break;
        case -4:
            errorMsg = "User not found";
            break;
        default:
            errorMsg = "Token parsing error";
            break;
        }
        res = ResponseHelper::unauthorized(req, errorMsg);
        return -1;
    }

    // 4. 验证数据库连接
    if (!dbManager || !dbManager->getSession() || !dbManager->getSchema()) {
        res = ResponseHelper::system_error(req, "Database connection unavailable");
        return -1;
    }

    // 5. 验证用户权限
    if (!JwtUtils::isUserAuthorizedForOrder(userId, orderId, dbManager))
    {
        res = ResponseHelper::unauthorized(req, "Access denied to this order");
        return -1;
    }

    return userId;
}

// 验证超级管理员token
int isValidSuperAdminToken(const crow::request &req, crow::response &res, std::shared_ptr<DatabaseManagerInterface> dbManager)
{
    return validateTokenWithScope(req, res, dbManager, TokenValidationScope::SuperAdmin);
}

crow::response authHandler::authCheckName(const crow::request &req)
{
    try
    {

        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string name = "";
        bool hasName = (request_body.find("name") != request_body.end());
        if (hasName)
        {
            name = request_body["name"].is_string() ? request_body["name"].get<std::string>() : request_body["name"].dump();
        }
        else
        {
            return ResponseHelper::error(req, "Missing name parameter");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT COUNT(*) as count FROM users WHERE name = ?")
                                       .bind(name)
                                       .execute();

        mysqlx::Row row = result.fetchOne();
        if (row && !row[0].isNull())
        {
            int count = row[0].get<int>();
            if (count == 0) // 说明用户名没有被注册
            {
                return ResponseHelper::success(req, "username is not used");
            }
            else // 说明用户名被注册(既无法继续注册)
            {
                return ResponseHelper::error(req, "username is used");
            }
        }
        else
        {
            // 查询失败
            return ResponseHelper::system_error(req);
        }
    }
    catch (const mysqlx::Error &e)
    {
        return ResponseHelper::database_error(req, "Database error", e.what());
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req);
    }
}

crow::response authHandler::authCheckEmail(const crow::request &req)
{
    try
    {

        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string email = "";
        bool hasEmail = (request_body.find("email") != request_body.end());
        if (hasEmail)
        {
            email = request_body["email"].is_string() ? request_body["email"].get<std::string>() : request_body["email"].dump();
        }
        else
        {
            return ResponseHelper::error(req, "Missing email parameter");
        }

        email = clean_string(email);

        // 添加邮箱格式验证
        if (!isValidEmailFormat(email))
        {
            return ResponseHelper::error(req, "Invalid email format");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT COUNT(*) as count FROM users WHERE email = ?")
                                       .bind(email)
                                       .execute();

        auto row = result.fetchOne();
        if (row && !row[0].isNull())
        {
            int count = row[0].get<int>();
            if (count == 0) // 说明邮件没有被注册
            {
                return ResponseHelper::success(req, "emailAddress is not used");
            }
            else // 说明邮件被注册(既无法继续注册)
            {
                return ResponseHelper::error(req, "emailAddress is used");
            }
        }
        else
        {
            // 查询失败
            return ResponseHelper::system_error(req);
        }
    }
    catch (const mysqlx::Error &e)
    {
        return ResponseHelper::database_error(req, "Database error", e.what());
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req);
    }
}

crow::response authHandler::authCheckPhone(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string phone = "";
        bool hasPhone = (request_body.find("phone") != request_body.end());
        if (hasPhone)
        {
            phone = request_body["phone"].is_string() ? request_body["phone"].get<std::string>() : request_body["phone"].dump();
        }
        else
        {
            return ResponseHelper::validation(req, "Missing phone parameter");
        }
        phone = clean_string(phone);

        if (!isValidPhoneFormat(phone))
        {
            return ResponseHelper::validation(req, "Invalid phone format");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT COUNT(*) as count FROM users WHERE phone = ?")
                                       .bind(phone)
                                       .execute();

        auto row = result.fetchOne();
        if (row && row[0].isNull())
        {
            int count = row[0].get<int>();
            if (count == 0) // 说明电话号码没有被注册，可以继续注册
            {
                return ResponseHelper::success(req, "phoneNumber is not used");
            }
            else // 说明电话号码被注册(既无法继续注册)
            {
                return ResponseHelper::error(req, "phoneNumber is used");
            }
        }
        else
        {
            // 查询失败
            return ResponseHelper::system_error(req);
        }
    }
    catch (const mysqlx::Error &e)
    {
        return ResponseHelper::database_error(req, "Database error", e.what());
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req);
    }
}

// 准备验证码函数
crow::response authHandler::authReadyVerification(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        if (request_body.find("email") == request_body.end())
        {
            return ResponseHelper::error(req, "Missing email parameter");
        }

        std::string email = request_body["email"].is_string() ? request_body["email"].get<std::string>() : request_body["email"].dump();

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT * FROM users WHERE email = ?")
                                       .bind(email)
                                       .execute();

        if (result.begin() == result.end()) // 没有匹配的用户才创建验证码并发送邮件
        {
            Verify verify(email); // 栈上的对象

            // 检查邮箱地址
            if (verify.VerifyEmailAddress(email) == false)
            {
                return ResponseHelper::error(req, "emailAddress is in wrong format");
            }

            // 创建验证码
            verify.CreateVerify();

            // 使用智能指针的主要原因是：
            // 资源共享 - 多个线程或作用域需要访问同一个对象
            // 自动内存管理 - 避免内存泄漏和手动内存管理的复杂性
            // 线程安全 - 确保对象在需要时不会被提前销毁
            // 异常安全 - 即使发生异常也能正确释放资源
            auto email_ptr = std::make_shared<std::string>(email);
            auto verify_ptr = std::make_shared<Verify>(verify); // 创建Verify对象，通过拷贝构造创建堆上的对象

            // 异步编程和承诺/未来模式 (Promise/Future)
            auto promise_ptr = std::make_shared<std::promise<bool>>();
            auto future = promise_ptr->get_future(); // 从promise获取future

            // 发送邮件验证码
            std::thread sender([email_ptr, verify_ptr, promise_ptr]()
                               {
                        try {
                            verify_ptr->SendVerify(*email_ptr, verify_ptr->GetVerifyCode(), promise_ptr.get());
                        } catch (...) {
                            promise_ptr->set_value(false); // 确保在异常情况下也设置结果
                        } });
            sender.detach();

            // 等待邮件发送步骤完成，判断发送结果
            nlohmann::json response;
            try
            {
                bool sendSuccess = future.get(); // 使用之前获取的future对象
                if (sendSuccess)                 // 使用之前获取的future对象
                {
                    response["sent"] = true;
                    response["channel"] = "email";
                    return ResponseHelper::success(req, response);
                }
                else
                {
                    return ResponseHelper::error(req, "failed to send verification code email");
                }
            }
            catch (const std::exception &e)
            {
                // 这个catch块会捕获所有继承自std::exception的异常类型，包括：
                // std::runtime_error（运行时错误）
                // std::logic_error（逻辑错误）
                // std::invalid_argument（无效参数）
                // std::out_of_range（超出范围）
                // 其他标准库抛出的异常
                return ResponseHelper::system_error(
                    req,
                    "exception occurred while sending email: " + std::string(e.what()));
            }
            catch (...)
            {
                return ResponseHelper::system_error(req);
            }
        }
        else
        {
            return ResponseHelper::error(req, "emailAddress is already in use");
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req);
    }
}

// 邮箱验证码验证函数
crow::response authHandler::checkVerifyEmailCode(const crow::request &req)
{
    try
    {
        crow::response res;
        auto jsonOpt = parseJson(req, res);
        nlohmann::json &request_body = jsonOpt.value();

        // 检查必要字段是否存在
        if (request_body.find("email") == request_body.end() ||
            request_body.find("code") == request_body.end())
        {
            return ResponseHelper::error(req, "verify : false , message : Missing email or code");
        }

        std::string email = request_body["email"];
        std::string code = request_body["code"];
        int userID = 0;
        std::string userName = "";

        // 创建验证码对象
        Verify verify(email);

        // 验证码验证
        bool isValid = Verify::ValidateCode(email, code);

        // 验证码验证
        if (isValid)
        {
            mysqlx::SqlResult result = dbManager->getSession()
                                           ->sql("SELECT id, username FROM users WHERE email = ?")
                                           .bind(email)
                                           .execute();

            for (const auto &row : result)
            {
                userID = row[0].get<int>();
                userName = clean_string(row[1].get<std::string>());
            }

            // 如果验证成功，返回 token
            nlohmann::json response;
            // 生成一个基于用户邮箱的JWT token
            const int defaultUserRoleId =
                RoleTypeUtils::getRoleId(dbManager, "普通用户");
            std::string token = JwtUtils::createToken(
                userID,
                userName,
                defaultUserRoleId,
                "普通用户",
                email,
                true);
            response["token"] = token;
            return ResponseHelper::success(req, response);
        }
        else
        {
            return ResponseHelper::verification_failed(
                req,
                "Invalid Verification Code",
                "Verification code validation failed");
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req);
    }
}

// 刷新管理员令牌
crow::response authHandler::refreshAdminToken(const crow::request &req)
{
    try
    {
        std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
        {
            return ResponseHelper::unauthorized(req, "Missing or invalid token");
        }

        std::string token = authHeader.substr(7);
        auto claims = JwtUtils::getTokenClaims(token);
        if (!claims || claims->userId <= 0 || claims->identifier.empty())
        {
            return ResponseHelper::unauthorized(req, "Token expired or invalid");
        }

        int userId = claims->userId;
        const std::string &identifier = claims->identifier;

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT u.type_id, t.type, u.name, u.email, u.phone "
                                             "FROM users AS u "
                                             "JOIN types AS t ON u.type_id = t.id "
                                             "WHERE u.id = ?")
                                       .bind(userId)
                                       .execute();

        auto row = result.fetchOne();
        if (!row)
        {
            return ResponseHelper::unauthorized(req, "Admin user not found");
        }

        int typeId = row[0].get<int>();
        std::string typeName = row[1].get<std::string>();
        if (typeName != "超级管理员")
        {
            return ResponseHelper::unauthorized(req, "Only super admin can refresh this token");
        }

        std::string userName = clean_string(row[2].get<std::string>());
        std::string email = row[3].isNull() ? "" : clean_string(row[3].get<std::string>());
        std::string phone = row[4].isNull() ? "" : normalizePhoneIdentifier(row[4].get<std::string>());

        bool isEmailLogin = !email.empty() && identifier == email;
        bool isPhoneLogin = !phone.empty() && normalizePhoneIdentifier(identifier) == phone;

        if (!isEmailLogin && !isPhoneLogin)
        {
            return ResponseHelper::unauthorized(req, "Token identifier does not match admin account");
        }

        nlohmann::json response;
        response["token"] = JwtUtils::createToken(
            userId,
            userName,
            typeId,
            typeName,
            isEmailLogin ? email : phone,
            isEmailLogin);
        response["expiresIn"] = 300;
        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Failed to refresh admin token: " << e.what() << std::endl;
        return ResponseHelper::system_error(req);
    }
}

// 执行Python脚本发送短信验证码
std::pair<bool, std::string> executePythonScript(const std::string &phone, const std::string &code)
{
    // 获取Python脚本路径
    std::string script_path = "/Users/yanghang/Code/PetManager/pethospital/backend/controllers/auth/Verification/SendSmsVerifyCode.py";

    // 构建命令
    std::string command = "python3 \"" + script_path + "\" \"" + phone + "\" \"" + code + "\" --json 2>&1";

    std::cout << "Executing command: " << command << std::endl;

    // 执行命令
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe)
    {
        std::cout << "Failed to execute python script: popen() failed" << std::endl;
        return std::make_pair(false, "Failed to execute python script");
    }

    // 读取输出
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    pclose(pipe.release());

    // 解析JSON结果
    try
    {
        auto json_result = nlohmann::json::parse(result);
        bool success = json_result.value("success", false);
        std::string message = json_result.value("message", "");

        std::cout << "[INFO] Python script result - Success: " + std::to_string(success) + ", Message: " + message << std::endl;

        return std::make_pair(success, message);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Failed to parse JSON result: " + std::string(e.what()) << std::endl;
        std::cerr << "[ERROR] Raw result: " + result << std::endl;
        return std::make_pair(false, "Failed to parse script result: " + result);
    }
}

// 手机号验证码发送函数
crow::response authHandler::sendSmsVerification(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        // 验证必要参数
        if (request_body.find("phone") == request_body.end())
        {
            return ResponseHelper::error(req, "Missing phone parameter");
        }

        std::string phone = request_body["phone"].is_string() ? request_body["phone"].get<std::string>() : request_body["phone"].dump();

        phone = clean_string(phone);

        // 验证手机号格式
        if (!isValidPhoneFormat(phone))
        {
            return ResponseHelper::validation(req, "Invalid phone format");
        }

        // 检查手机号是否已被注册
        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT COUNT(*) as count FROM users WHERE phone = ?")
                                       .bind(phone)
                                       .execute();

        auto row = result.fetchOne();
        if (row && !row[0].isNull())
        {
            int count = row[0].get<int>();
            if (count > 0)
            {
                return ResponseHelper::error(req, "phoneNumber is already in use");
            }
        }

        // 生成验证码
        Verify verify(phone);
        std::string code = verify.CreateVerify();

        std::cout << "[INFO] Generated verification code for phone: " + phone + ", code: " + code << std::endl;

        // 异步发送短信
        auto phone_ptr = std::make_shared<std::string>(phone);
        auto code_ptr = std::make_shared<std::string>(code);
        auto promise_ptr = std::make_shared<std::promise<std::pair<bool, std::string>>>();
        auto future = promise_ptr->get_future();

        std::thread sender([phone_ptr, code_ptr, promise_ptr]()
                           {
            try 
            {
                auto result = executePythonScript(*phone_ptr, *code_ptr);
                promise_ptr->set_value(result);
            } 
            catch (const std::exception& e) 
            {
                std::cerr << "[ERROR] Exception in SMS sender thread: " + std::string(e.what()) << std::endl;
                promise_ptr->set_value(std::make_pair(false, "Exception: " + std::string(e.what())));
            }
            catch (...) 
            {
                std::cerr << "[ERROR] Unknown exception in SMS sender thread" << std::endl;
                promise_ptr->set_value(std::make_pair(false, "Unknown exception"));
            } });
        sender.detach();

        // 等待发送结果
        nlohmann::json response;
        try
        {
            auto send_result = future.get();
            bool send_success = send_result.first;
            std::string message = send_result.second;

            if (send_success)
            {
                response["sent"] = true;
                response["channel"] = "sms";
                response["phone"] = phone; // 可以考虑隐藏部分号码
                std::cout << "[INFO] SMS verification sent successfully to: " + phone << std::endl;
                return ResponseHelper::success(req, response);
            }
            else
            {
                std::cerr << "[ERROR] Failed to send SMS verification to: " + phone + ", reason: " + message << std::endl;
                return ResponseHelper::error(req, "发送验证码失败: " + message);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] System exception in sendSmsVerification: " + std::string(e.what()) << std::endl;
            return ResponseHelper::system_error(req, "系统异常: " + std::string(e.what()));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Exception in sendSmsVerification: " + std::string(e.what()) << std::endl;
        return ResponseHelper::system_error(req);
    }
}

// 手机号验证码验证函数
crow::response authHandler::checkVerifySmsCode(const crow::request &req)
{
    try
    {
        crow::response res;
        auto jsonOpt = parseJson(req, res);
        nlohmann::json &request_body = jsonOpt.value();


        // 检查必要字段
        if (request_body.find("phone") == request_body.end() ||
            request_body.find("code") == request_body.end())
        {
            return ResponseHelper::error(req, "Missing phone or code parameter");
        }

        std::string phone = request_body["phone"];
        std::string code = request_body["code"];
        int userID = 0;
        std::string userName = "";

        // 验证验证码
        bool isValid = Verify::ValidateCode(phone, code);

        if (isValid)
        {
            mysqlx::SqlResult result = dbManager->getSession()
                                           ->sql("SELECT id, name FROM users WHERE phone = ?")
                                           .bind(phone)
                                           .execute();
            for (const auto &row : result)
            {
                userID = row[0].get<int>();
                userName = clean_string(row[1].get<std::string>());
            }
            // 验证成功，返回token
            nlohmann::json response;
            const int defaultUserRoleId =
                RoleTypeUtils::getRoleId(dbManager, "普通用户");
            std::string token = JwtUtils::createToken(
                userID,
                userName,
                defaultUserRoleId,
                "普通用户",
                phone,
                false);
            response["token"] = token;
            return ResponseHelper::success(req, response);
        }
        else
        {
            return ResponseHelper::verification_failed(
                req,
                "Invalid Verification Code",
                "Verification code validation failed");
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req);
    }
}
