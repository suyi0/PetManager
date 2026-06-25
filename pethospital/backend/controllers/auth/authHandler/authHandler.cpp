#include "authHandler.h"
#include "../smsScriptRunner.h"

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
                                       ->sql("SELECT COUNT(*) as count FROM phones WHERE phone = ?")
                                       .bind(phone)
                                       .execute();

        auto row = result.fetchOne();
        if (row && !row[0].isNull())
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

// 发送邮件验证码函数
crow::response authHandler::getEmailVerification(const crow::request &req, int userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string email = getRequestString(request_body, "email", "");

        if (email.empty())
        {
            return ResponseHelper::error(req, "邮箱不能为空");
        }

        if (!isValidEmailFormat(email)) // 邮箱格式验证
        {
            return ResponseHelper::error(req, "邮箱格式错误");
        }

        if (userId <= 0) // 注册时获取验证码
        {
            mysqlx::SqlResult emailResult = dbManager->getSession()
                                                ->sql("SELECT id "
                                                      "FROM users "
                                                      "WHERE email = ? AND is_deleted = 0 "
                                                      "LIMIT 1")
                                                .bind(email)
                                                .execute();

            if (emailResult.fetchOne())
            {
                return ResponseHelper::error(req, "该邮箱已被注册");
            }
        }
        else // 修改邮箱时获取验证码
        {
            mysqlx::SqlResult currentUserResult = dbManager->getSession()
                                                      ->sql("SELECT email "
                                                            "FROM users "
                                                            "WHERE id = ? AND is_deleted = 0 "
                                                            "LIMIT 1")
                                                      .bind(userId)
                                                      .execute();

            auto currentUserRow = currentUserResult.fetchOne();
            if (!currentUserRow) // 正常不会到这
            {
                return ResponseHelper::notFound(req, "用户不存在");
            }

            const std::string DBEmail = currentUserRow[0].isNull() ? "" : currentUserRow[0].get<std::string>();
            if (email == DBEmail)
            {
                return ResponseHelper::error(req, "新邮箱不能与当前邮箱相同");
            }

            mysqlx::SqlResult emailOwnerResult = dbManager->getSession()
                                                     ->sql("SELECT id "
                                                           "FROM users "
                                                           "WHERE email = ? AND id <> ? AND is_deleted = 0 "
                                                           "LIMIT 1")
                                                     .bind(email, userId)
                                                     .execute();

            if (emailOwnerResult.fetchOne())
            {
                return ResponseHelper::error(req, "该邮箱已被其他用户使用");
            }
        }

        std::string code = Verify::CreateEmailVerify(email);

        // 使用智能指针的主要原因是：
        // 资源共享 - 多个线程或作用域需要访问同一个对象
        // 自动内存管理 - 避免内存泄漏和手动内存管理的复杂性
        // 线程安全 - 确保对象在需要时不会被提前销毁
        // 异常安全 - 即使发生异常也能正确释放资源
        auto email_ptr = std::make_shared<std::string>(email);
        auto code_ptr = std::make_shared<std::string>(code);

        // 异步编程和承诺/未来模式 (Promise/Future)
        auto promise_ptr = std::make_shared<std::promise<bool>>();
        auto future = promise_ptr->get_future(); // 从promise获取future

        // 发送邮件验证码(异步)
        std::thread sender([email_ptr, code_ptr, promise_ptr]()
                           {
            try
            {
                Verify::SendEmailVerify(*email_ptr, *code_ptr, promise_ptr.get());
            }
            catch (...) {
                promise_ptr->set_value(false); // 确保在异常情况下也设置结果
            } });

        sender.detach(); // 将线程与主线程分离，使其在后台独立运行

        // 等待邮件发送步骤完成，判断发送结果
        nlohmann::json response;
        try
        {
            bool sendSuccess = future.get(); // 使用之前获取的future对象,获取异步操作的结果
            if (sendSuccess)
            {
                response["sent"] = true;
                response["channel"] = "email";
                response["scene"] = userId <= 0 ? "register" : "change";
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
            return ResponseHelper::system_error(req, "exception occurred while sending email: " + std::string(e.what()));
        }
        catch (...)
        {
            return ResponseHelper::system_error(req);
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req);
    }
}

// 验证邮箱验证码函数
crow::response authHandler::checkVerifyEmailCode(const crow::request &req, int userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        // 检查必要字段是否存在
        if (!request_body.contains("email") || !request_body.contains("code"))
        {
            return ResponseHelper::error(req, "缺少邮箱或验证码");
        }

        if (!isValidEmailFormat(request_body["email"]))
        {
            return ResponseHelper::error(req, "邮箱格式错误");
        }

        std::string email = getRequestString(request_body, "email", "");
        std::string code = getRequestString(request_body, "code", "");

        bool isValid = Verify::ValidateCode(email, code); // 验证码验证

        // 验证码验证成功
        if (isValid)
        {
            nlohmann::json response;
            response["success"] = true;
            if (userId > 0)
            {
                response["ticket"] = JwtUtils::createUpdateTicket(userId, email, "email");
            }
            return ResponseHelper::success(req, response);
        }
        else
        {
            return ResponseHelper::verification_failed(req, "Invalid Verification Code", "Verification code validation failed");
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
                                       ->sql("SELECT u.type_id, t.type, u.email, p.phone "
                                             "FROM users AS u "
                                             "JOIN types AS t ON u.type_id = t.id "
                                             "LEFT JOIN phones AS p ON p.user_id = u.id "
                                             "WHERE u.id = ? AND u.is_deleted = 0")
                                       .bind(userId)
                                       .execute();

        auto row = result.fetchOne();
        if (!row)
        {
            return ResponseHelper::unauthorized(req, "Admin user not found");
        }

        int typeId = row[0].get<int>();
        std::string typeName = row[1].get<std::string>();
        if (!RoleTypeUtils::isManagementRole(typeName))
        {
            return ResponseHelper::unauthorized(req, "Only management roles can refresh this token");
        }

        std::string email = row[2].isNull() ? "" : row[2].get<std::string>();
        std::string phone = row[3].isNull() ? "" : normalizePhoneIdentifier(row[3].get<std::string>());

        bool isEmailLogin = !email.empty() && identifier == email;
        bool isPhoneLogin = !phone.empty() && normalizePhoneIdentifier(identifier) == phone;

        if (!isEmailLogin && !isPhoneLogin)
        {
            return ResponseHelper::unauthorized(req, "Token identifier does not match admin account");
        }

        nlohmann::json response;
        response["token"] = JwtUtils::createToken(
            userId,
            typeId,
            typeName,
            isEmailLogin ? email : phone,
            isEmailLogin);
        response["expiresIn"] = 1800;
        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] Failed to refresh admin token: " << e.what() << std::endl;
        return ResponseHelper::system_error(req);
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
                                       ->sql("SELECT COUNT(*) as count FROM phones WHERE phone = ?")
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
        std::string code = Verify::CreatePhoneVerify(phone);

        // 异步发送短信
        auto phone_ptr = std::make_shared<std::string>(phone);
        auto code_ptr = std::make_shared<std::string>(code);
        auto promise_ptr = std::make_shared<std::promise<std::pair<bool, std::string>>>();
        auto future = promise_ptr->get_future();

        std::thread sender([phone_ptr, code_ptr, promise_ptr]()
                           {
            try 
            {
                auto result = SmsScriptRunner::execute(*phone_ptr, *code_ptr);
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
crow::response authHandler::checkVerifySmsCode(const crow::request &req, int userId)
{
    try
    {
        crow::response res;
        auto jsonOpt = parseJson(req, res);
        if (!jsonOpt)
            return res;
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

        // 验证验证码
        bool isValid = Verify::ValidateCode(phone, code);

        if (isValid)
        {
            // 验证成功，返回token
            nlohmann::json response;
            response["success"] = true;
            if (userId > 0)
            {
                response["ticket"] = JwtUtils::createUpdateTicket(userId, phone, "phone");
            }
            return ResponseHelper::success(req, response);
        }
        else
        {
            return ResponseHelper::verification_failed(req, "Invalid Verification Code", "Verification code validation failed");
        }
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req);
    }
}
