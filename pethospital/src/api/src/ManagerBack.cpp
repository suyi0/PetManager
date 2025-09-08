#include "../include/ManagerBack.h"

// 定义全局数据库变量
mysqlx::Session *g_db_session = nullptr;
mysqlx::Schema *g_database = nullptr;

// 添加保存地址到数据库的函数
std::string saveAddressToDatabase(const std::string &address_text, double longitude, double latitude)
{
    try
    {
        // 获取 address_small 表
        mysqlx::Table address_table = g_database->getTable("address_small");

        // 获取当前最大ID值
        int new_id = 1; // 默认ID为1
        try
        {
            mysqlx::RowResult result = address_table.select("MAX(id)").execute();
            auto row = result.fetchOne();
            if (row)
            {
                auto max_id = row[0];
                if (!max_id.isNull())
                {
                    new_id = max_id.get<int>() + 1;
                }
            }
        }
        catch (const std::exception &)
        {
            // 如果获取最大ID失败，就使用默认值1
            new_id = 1;
        }

        // 插入新的地址记录
        mysqlx::TableInsert insert_op = address_table.insert("id", "name", "longitude", "latitude");
        insert_op.values(new_id, address_text, longitude, latitude).execute();

        return std::to_string(new_id); // 返回新插入记录的ID
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error saving address to database: " << e.what() << std::endl;
        return "0"; // 返回0表示失败
    }
}

void WebSocketServer::start()
{
    setupRoutes();          // 设置路由
    setupSignalHandlers();  // 设置信号处理
    startCodeCleanupTask(); // 启动定时任务

    server_thread = std::thread([this]
                                {
            try {
                app.port(8081).multithreaded().run();
            } catch (const std::exception& e) {
                std::cerr << "Server fatal error: " << e.what() << std::endl;
            } });
}

void WebSocketServer::startCodeCleanupTask()
{
    cleanup_thread = std::thread([]()
                                 {
            while (true) {
                // 每隔一段时间清理一次过期验证码
                std::this_thread::sleep_for(std::chrono::minutes(5));
                Verify::CleanupExpiredCodes();
            } });
    cleanup_thread.detach();
}

void WebSocketServer::gracefulShutdown()
{
    heartbeat_running = false;
    if (heartbeat_thread.joinable())
        heartbeat_thread.join();

    std::cout << "Initiating graceful shutdown..." << std::endl;

    // 关闭所有活跃连接 [1,5](@ref)
    {
        std::lock_guard<std::mutex> lock(conn_mutex);
        // 创建连接副本以避免在迭代时修改集合
        auto connections_copy = active_connections;
        for (auto *conn : connections_copy)
        {
            if (conn)
            {
                try
                {
                    conn->close("server_shutdown");
                }
                catch (...)
                {
                    // 忽略关闭连接时的异常
                }
            }
        }
    }

    // 等待连接关闭（仅检测active_connections为空）
    std::unique_lock<std::mutex> lk(conn_mutex);
    shutdown_cv.wait(lk, [this]
                     {
                         return active_connections.empty(); // 仅等待连接清空
                     });
    // 停止服务器
    app.stop();
    if (server_thread.joinable())
    {
        server_thread.join();
    }
    stop_requested = true;
    std::cout << "Server shutdown complete" << std::endl;
}

// 实现WebSocketServer的单例模式instance方法
WebSocketServer &WebSocketServer::instance()
{
    static WebSocketServer instance;
    return instance;
}

// 提供公共方法访问 signal_received
bool WebSocketServer::isSignalReceived() const
{
    return signal_received.load();
    /*load() 方法支持显式指定内存顺序（默认为std::memory_order_seq_cst）。该顺序要求：
    当前线程的后续操作不会重排到load()之前。
    其他线程对同一原子的写入对所有线程可见。
    在信号处理场景中，这确保主线程能立即感知到信号标志的变化，避免因编译器/CPU指令重排导致延迟可见。*/
}

// 设置路由
void WebSocketServer::setupRoutes()
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
        return;
    routes_setup = true;

    // 添加登录路由
    CROW_ROUTE(app, "/api/user/login")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([](const crow::request &req, crow::response &res)
                                                                    {
                // 处理OPTIONS预检请求
                initializeOPTIONS(req,res);

                // 处理POST请求
                // 解析请求体中的 JSON 数据
                nlohmann::json request_body;
                try
                {
                    request_body = nlohmann::json::parse(req.body);
                }
                catch (...)
                {
                    res.code = 400;
                    initializeCORS(req,res);
                    res.write(R"({"error": "Invalid JSON"})");
                    res.end(); // 显式结束响应
                    return;
                }

                // 检查必要字段是否存在
                bool hasEmail = (request_body.find("email") != request_body.end());
                bool hasPhone = (request_body.find("phone") != request_body.end());
                bool hasPassword = (request_body.find("password") != request_body.end());

                 // 检查是否提供了邮箱或电话号码，并且提供了密码
                if (!hasPassword && (!hasEmail || !hasPhone)) {
                    res.code = 400;
                    initializeCORS(req,res);
                    res.write(R"({"error": "Missing email or password"})");
                    res.end();
                    return;
                }

                std::string email = "";
                std::string phone = "";
                std::string password = "";

               // 安全地获取字段值
                if (hasEmail) {
                    email = request_body["email"].is_string() ? 
                            request_body["email"].get<std::string>() : 
                            request_body["email"].dump();
                }
                
                if (hasPhone) {
                    phone = request_body["phone"].is_string() ? 
                            request_body["phone"].get<std::string>() : 
                            request_body["phone"].dump();
                }
                
                if (hasPassword) {
                    password = request_body["password"].is_string() ? 
                               request_body["password"].get<std::string>() : 
                               request_body["password"].dump();
                }

                // 输出调试信息
                std::cout << "request_body" << req.body << std::endl; 
                std::cout << "email: " << email << std::endl;
                std::cout << "phone: " << phone << std::endl;
                std::cout << "password: " << password << std::endl;

                // 对输入的密码进行SHA-256哈希处理
                std::string hashed_password = sha256_hash(password);

                // 检查数据库连接是否存在
                if (!g_db_session || !g_database) {
                    res.code = 500;
                    // 添加CORS头
                    initializeCORS(req,res);

                    res.write(R"({"error": "Database connection not available"})");
                    res.end();
                    return;
                }

                // 从数据库中获取用户信息
                //user是一个智能指针
                std::unique_ptr<User> user = nullptr;
                try {
                    // 获取表
                    mysqlx::Table users_table = g_database->getTable("users");
                    
                    // 查询用户
                    mysqlx::RowResult result;
                    if (!email.empty()) {
                        // 通过email查询用户
                        result = users_table.select("id", "name", "password", "phone", "email", "CAST(birthday AS CHAR)", "creation_time", "address_id")
                                            .where("email = :email")
                                            .bind("email", email)
                                            .execute();
                    } else if (!phone.empty()) {
                        // 通过phone查询用户
                        result = users_table.select("id", "name", "password", "phone", "email", "CAST(birthday AS CHAR)", "creation_time", "address_id")
                                            .where("phone = :phone")
                                            .bind("phone", phone)
                                            .execute();
                    } else {
                        // 理论上不会到达这里，因为前面已经检查过了
                        res.code = 500;
                        res.set_header("Content-Type", "application/json");
                        initializeCORS(req, res);
                        res.write(R"({"error": "Either email or phone must be provided"})");
                        res.end();
                        return;
                    }

                    // 即使email变量包含恶意代码，也会被当作普通字符串值处理
                    // 处理结果
                    for (auto row : result) {
                        user = std::make_unique<User>();
                        user->id = row[0].get<int>();
                        // 确保正确处理所有字段，添加错误检查
                        try {
                            user->name = clean_string(row[1].get<std::string>());
                        } catch (...) { 
                            user->name = "[Invalid Name]";
                        }
                        
                        try {
                            user->password = clean_string(row[2].get<std::string>());
                        } catch (...) {
                            user->password = "";
                        }
                        
                        try {
                            user->phone = clean_string(row[3].get<std::string>());
                        } catch (...) {
                            user->phone = "";
                        }

                        try {
                            user->email = clean_string(row[4].get<std::string>());
                        } catch (...) {
                            user->email = "";
                        }

                        // 处理生日字段，确保其格式正确
                        try {
                        auto birthday_value = row[5];
                        if (birthday_value.isNull()) {
                            user->birthday = boost::gregorian::date(1970, 1, 1);
                        } else {
                            // 使用 toString() 方法显式转换为字符串
                            std::string birthday_str = birthday_value.get<std::string>();
                            
                            // 清理字符串，移除可能的空格或不可见字符
                            birthday_str = clean_string(birthday_str);
                            
                            // 确保格式为 YYYY-MM-DD
                            if (birthday_str.length() == 10 && birthday_str[4] == '-' && birthday_str[7] == '-') {
                                try {
                                    user->birthday = boost::gregorian::from_simple_string(birthday_str);
                                    std::cout << "Debug: Successfully parsed birthday: " << birthday_str << std::endl;
                                } catch (...) {
                                    std::cout << "Debug: Failed to parse birthday: " << birthday_str << ", using default" << std::endl;
                                    user->birthday = boost::gregorian::date(1970, 1, 1);
                                }
                            } else {
                                std::cout << "Debug: Invalid birthday format: " << birthday_str << ", using default" << std::endl;
                                user->birthday = boost::gregorian::date(1970, 1, 1);
                            }
                        }
                    } catch (const std::exception& e) {
                        std::cout << "Debug: Exception in birthday processing: " << e.what() << std::endl;
                        user->birthday = boost::gregorian::date(1970, 1, 1);
                    }

                        try {
                            if (!row[7].isNull()) {
                                user->address_id = clean_string(row[7].get<std::string>());
                            } else {
                                user->address_id = "1";  // 默认地址ID
                            }
                        } catch (...) {
                            user->address_id = "1";  // 默认地址ID
                        }

                        break; // 只需要第一个匹配的用户
                    }
                } catch (const mysqlx::Error& e) {
                    // 数据库操作错误
                    std::cerr << "Database error: " << e.what() << std::endl;
                    res.code = 500;
                    res.set_header("Content-Type", "application/json");
                   // 添加CORS头
                    initializeCORS(req,res);

                    res.write(R"({"error": "Database operation failed", "details": ")" + std::string(e.what()) + "\"}");
                } catch (const std::exception& e) {
                    // 其他错误
                    std::cerr << "Error: " << e.what() << std::endl;
                    res.code = 500;
                    // 添加CORS头
                    initializeCORS(req,res);

                    res.write(R"({"error": "Operation failed", "details": ")" + std::string(e.what()) + "\"}");
                }

                // 在这里验证用户名和密码 (示例验证)
                nlohmann::json response;
                if (!user || user->password != hashed_password) {
                    // 不区分用户不存在和密码错误，统一返回相同错误信息
                    response["error"] = "Invalid username or password";
                    response["success"] = false;
                    res.code = 401;
                } else {
                    // 验证成功
                    // 生成一个基于用户邮箱的JWT token
                    std::string token = generate_SHA256_jwt(email);
                    response["token"] = token;
                    response["success"] = true;
                    
                    // 手动构建用户JSON对象，确保birthday正确序列化
                    nlohmann::json user_json;
                    user_json["id"] = user->id;
                    user_json["name"] = user->name;
                    user_json["email"] = user->email;
                    user_json["phone"] = user->phone;
                    user_json["address_id"] = user->address_id;
                    
                    // 特别处理birthday字段，将其转换为字符串格式
                    std::ostringstream oss;
                    oss << std::setfill('0') << std::setw(4) << user->birthday.year()
                        << "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(user->birthday.month())
                        << "-" << std::setfill('0') << std::setw(2) << user->birthday.day();
                    user_json["birthday"] = oss.str();
                    
                    response["user"] = user_json;
                    res.code = 200;
                }
                initializeCORS(req,res);
                res.write(response.dump());
                res.end(); });

    // 添加根路径测试路由
    CROW_ROUTE(app, "/")
    ([]()
     { return "Server is running!"; });

    // 添加控制验证准备状态的路由
    CROW_ROUTE(app, "/api/verification/ready")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([](const crow::request &req, crow::response &res)
                                                                    {
                // 处理OPTIONS请求
                initializeOPTIONS(req,res);
                if (res.is_completed())
                {
                    return; // 如果是OPTIONS请求，直接返回
                }

                // 解析请求体中的 JSON 数据
                nlohmann::json request_body;
                if(!parseJsonBody(req, res, request_body))
                {
                    return;         // JSON解析失败，直接返回
                }

                std::string email = request_body["email"];

                Verify verify(email);// 栈上的对象

                // 检查邮箱地址
                if (verify.VerifyEmailAddress(email) == false)
                {
                    res.code = 404;
                    // 添加CORS头
                    initializeCORS(req,res);

                    res.write(R"({"sent": false, "message" : "emailAddress is invalid"})");
                    res.end();  // 结束响应
                    return; // 确保函数不会继续执行
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
                
                //异步编程和承诺/未来模式 (Promise/Future)
                auto promise_ptr = std::make_shared<std::promise<bool>>();
                auto future = promise_ptr->get_future(); // 从promise获取future

                // 发送邮件验证码
                std::thread sender([email_ptr, verify_ptr, promise_ptr]() {
                    try {
                        verify_ptr->SendVerify(*email_ptr, verify_ptr->GetVerifyCode(), promise_ptr.get());
                    } catch (...) {
                        promise_ptr->set_value(false); // 确保在异常情况下也设置结果
                    }
                });
                sender.detach();

                // 等待邮件发送步骤完成，判断发送结果
                nlohmann::json response;
                try {
                    bool sendSuccess = future.get(); // 使用之前获取的future对象
                    if (sendSuccess) // 使用之前获取的future对象
                    {
                        response["data"] = true;
                        response["message"] = "sent verification code email";
                        res.code = 200;
                        // 添加CORS头
                        initializeCORS(req,res);
        
                        res.write(response.dump());
                    }
                    else
                    {
                        response["data"] = false;
                        response["message"] = "failed to send verification code email";
                        res.code = 500;
                        // 添加CORS头
                        initializeCORS(req,res);
        
                        res.write(response.dump());
                    }
                } catch (const std::exception& e) {
                    // 这个catch块会捕获所有继承自std::exception的异常类型，包括：
                    // std::runtime_error（运行时错误）
                    // std::logic_error（逻辑错误）
                    // std::invalid_argument（无效参数）
                    // std::out_of_range（超出范围）
                    // 其他标准库抛出的异常
                    response["data"] = false;
                    response["message"] = "exception occurred while sending email: " + std::string(e.what()) + "\"";
                    res.code = 500;
                    // 添加CORS头
                    initializeCORS(req,res);
                    res.write(response.dump());
                } catch (...) {
                    res.code = 500;
                    // 添加CORS头
                    initializeCORS(req,res);
        
                    res.write(R"({"data": false, "message" : "exception occurred while sending email"})");
                }
                
                res.end(); });

    //  用户注册验证码验证路由
    CROW_ROUTE(app, "/api/user/verify")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([](const crow::request &req, crow::response &res)
                                                                    {
                // 处理OPTIONS预检请求
                initializeOPTIONS(req,res);
                if (res.is_completed())
                {
                    return; // 如果是OPTIONS请求，直接返回
                }

                // 解析请求体中的 JSON 数据
                nlohmann::json request_body;
                if(!parseJsonBody(req, res, request_body))
                {
                    return;         // JSON解析失败，直接返回
                }

                // 检查必要字段是否存在
                if (request_body.find("email") == request_body.end() ||
                    request_body.find("code") == request_body.end())
                {
                    res.code = 400;
                    // 添加CORS头
                    initializeCORS(req,res);

                    res.write(R"({"verify" : false , "message" : "Missing email or code"})");
                    res.end();
                    return;
                }

                std::string email = request_body["email"];
                std::string code = request_body["code"];

                // 创建验证码对象
                Verify verify(email);

                // 验证码验证
                bool isValid = Verify::ValidateCode(email, code);
                std::cout << "验证结果: " << (isValid ? "成功" : "失败") << std::endl;

                // 验证码验证
                if (isValid)
                {
                    // 如果验证成功，返回 token
                    nlohmann::json response;
                    // 生成一个基于用户邮箱的JWT token
                    std::string token = generate_SHA256_jwt(email);
                    response["token"] = token;
                    response["success"] = true;
                    res.code = 200;
                    // 添加CORS头
                    initializeCORS(req,res);

                    res.write(response.dump());
                }
                else
                {
                    // 验证失败
                    nlohmann::json response;
                    response["error"] = "Invalid Verification Code";
                    response["success"] = false;
                    res.code = 401;
                    // 添加CORS头
                    initializeCORS(req,res);

                    res.write(response.dump());
                }
                res.end(); });

    // 添加保存表单数据的路由
    CROW_ROUTE(app, "/api/user/form")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([](const crow::request &req, crow::response &res)
                                                                    {
        // 处理OPTIONS预检请求
        initializeOPTIONS(req, res);

        // 检查数据库连接是否存在
        if (!g_db_session || !g_database)
        {
            res.code = 500;
            // 添加CORS头
            initializeCORS(req, res);

            res.write(R"({"error": "Database connection not available"})");
            res.end();
            return;
        }

        // 解析请求体中的 JSON 数据
        nlohmann::json request_body;
        try
        {
            request_body = nlohmann::json::parse(req.body);
        }
        catch (...)
        {
            res.code = 400;
            // 添加CORS头
            initializeCORS(req, res);

            res.write(R"({"error": "Invalid JSON"})");
            res.end();
            return;
        }

        try
        {
            // 将数据插入到数据库表中(获取打开对应的表)
            mysqlx::Table users_table = g_database->getTable("users");

            std::string name = "";
            std::string password = "";
            std::string phone = "";
            std::string email = "";
            std::string birthday = "";

            // 注册时存储的数据
            if (request_body.find("password") != request_body.end() && request_body.find("email") != request_body.end())
            {
                std::cout << "注册" << std::endl;

                // 获取当前最大ID值
                int new_id = 1; // 默认ID为1
                try
                {
                    // mysqlx::RowResult 类的成员方法，用于从查询结果中获取第一行数据
                    mysqlx::RowResult result = users_table.select("MAX(id)").execute();
                    //.execute()用于实际执行 SELECT 查询并返回结果集（mysqlx::RowResult 对象）

                    // 使用 fetchOne() 方法来获取这一行数据
                    auto row = result.fetchOne();
                    if (row)
                    {
                        auto max_id = row[0];
                        // 当max_id为空时，返回true
                        // 当max_id不为空时，返回false
                        if (!max_id.isNull())
                        {
                            new_id = max_id.get<int>() + 1;
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    // 如果获取最大ID失败，就使用默认值1
                    std::cerr << "Warning: Failed to get max ID, using default. Error: " << e.what() << std::endl;
                    new_id = 1;
                }
                // 创建数据库操作
                mysqlx::TableInsert insert_op = users_table.insert("id", "name", "phone", "password", "email", "CAST(birthday AS CHAR)", "creation_time", "address_id");

                // 从请求中获取数据并确保它们是字符串类型
                if (request_body.find("name") != request_body.end() && !request_body["name"].is_null())
                {
                    name = request_body["name"].is_string() ? request_body["name"].get<std::string>() : request_body["name"].dump();
                }
                else
                {
                    name = "";
                }
                if (request_body.find("password") != request_body.end() && !request_body["password"].is_null())
                {
                    password = request_body["password"].is_string() ? request_body["password"].get<std::string>() : request_body["password"].dump();
                }
                else
                {
                    password = "";
                }
                if (request_body.find("phone") != request_body.end() && !request_body["phone"].is_null())
                {
                    phone = request_body["phone"].is_string() ? request_body["phone"].get<std::string>() : request_body["phone"].dump();
                }
                else
                {
                    phone = "";
                }
                if (request_body.find("email") != request_body.end() && !request_body["email"].is_null())
                {
                    email = request_body["email"].is_string() ? request_body["email"].get<std::string>() : request_body["email"].dump();
                }
                else
                {
                    email = "";
                }
                if (request_body.find("birthday") != request_body.end() && !request_body["birthday"].is_null())
                {
                    birthday = request_body["birthday"].is_string() ? request_body["birthday"].get<std::string>() : request_body["birthday"].dump();
                }
                else
                {
                    birthday = "";
                }

                // 如果name为空，则使用邮箱地址作为用户名
                if (name.empty() && !email.empty())
                {
                    name = email;
                }
                // 如果name和email都为空，则使用手机号作为用户名
                else if (name.empty() && email.empty() && !phone.empty())
                {
                    name = phone;
                }
                // 如果name、email和phone都为空，则使用随机字符串作为用户名
                else if (name.empty() && email.empty() && phone.empty())
                {
                    name = "未命名";
                }

                // 注册时使用邮箱的没有设置手机号和生日时，设置为空字符串
                if (phone.empty())
                {
                    phone = "";
                }
                if (birthday.empty())
                {
                    birthday = "";
                }

                // 使用SHA-256对密码进行哈希处理
                std::string hashed_password = sha256_hash(password);

                // 获取注册时间并格式化为MySQL datetime格式
                auto now = std::chrono::system_clock::now();
                std::time_t time_now = std::chrono::system_clock::to_time_t(now);
                std::tm tm_now = *std::localtime(&time_now);
                std::ostringstream oss;
                oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
                std::string creation_time = oss.str();

                insert_op.values(new_id, name, phone, hashed_password, email, birthday, creation_time, "1").execute();
            }
            // 用于执行 INSERT 操作并将数据插入到数据库中。

            // 更新用户数据
            else if (
                (request_body.find("name") != request_body.end() ||
                 request_body.find("phone") != request_body.end() ||
                 request_body.find("email") != request_body.end() ||
                 request_body.find("birthday") != request_body.end() ||
                 request_body.find("address_id") != request_body.end()) &&
                 request_body.find("email") != request_body.end())
            {
                std::cout << "更新用户数据" << std::endl;
                std::cout << request_body.dump() << std::endl;

                // 从请求中获取数据并确保它们是字符串类型
                if (request_body.find("name") != request_body.end() && !request_body["name"].is_null())
                {
                    if (request_body["name"].is_string())
                    {
                        name = request_body["name"].get<std::string>();
                    }
                    else
                    {
                        name = request_body["name"].dump();
                    }
                }
                else
                {
                    name = "";
                }
                if (request_body.find("phone") != request_body.end() && !request_body["phone"].is_null())
                {
                    if (request_body["phone"].is_string())
                    {
                        phone = request_body["phone"].get<std::string>();
                    }
                    else
                    {
                        phone = request_body["phone"].dump();
                    }
                }
                else
                {
                    phone = "";
                }
                if (request_body.find("email") != request_body.end() && !request_body["email"].is_null())
                {
                    if (request_body["email"].is_string())
                    {
                        email = request_body["email"].get<std::string>();
                    }
                    else
                    {
                        email = request_body["email"].dump();
                    }
                }
                else
                {
                    email = "";
                }
                // 在保存表单数据的路由中处理生日字段
                if (request_body.find("birthday") != request_body.end() && !request_body["birthday"].is_null()) {
                    if (request_body["birthday"].is_string()) {
                        birthday = request_body["birthday"].get<std::string>();
                        std::cout << "Debug: Raw birthday from frontend: " << birthday << std::endl;
                        
                        // 标准化日期格式
                        try {
                            boost::gregorian::date parsed_date;
                            
                            // 尝试解析不同格式的日期
                            if (birthday.length() >= 8) {  // 至少像 2003-9-20 这样的格式
                                // 检查是否已经是标准的 YYYY-MM-DD 格式
                                if (birthday.length() == 10 && birthday[4] == '-' && birthday[7] == '-') {
                                    parsed_date = boost::gregorian::from_simple_string(birthday);
                                } 
                                // 检查是否是 YYYY-M-D 格式 (没有前导零)
                                else if (std::regex_match(birthday, std::regex(R"(^\d{4}-\d{1,2}-\d{1,2}$)"))) {
                                    parsed_date = boost::gregorian::from_simple_string(birthday);
                                }
                                // 检查是否是 YYYY-MMM-DD 格式 (如 2003-Sep-20)
                                else if (std::regex_match(birthday, std::regex(R"(^\d{4}-[A-Za-z]{3}-\d{1,2}$)"))) {
                                    std::istringstream iss(birthday);
                                    iss.imbue(std::locale(std::locale::classic(), 
                                          new boost::gregorian::date_input_facet("%Y-%b-%d")));
                                    iss >> parsed_date;
                                } 
                                // 其他情况尝试直接解析
                                else {
                                    parsed_date = boost::gregorian::from_simple_string(birthday);
                                }
                                
                                // 转换回标准的 YYYY-MM-DD 格式
                                std::ostringstream oss;
                                oss << std::setfill('0') << std::setw(4) << parsed_date.year()
                                    << "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(parsed_date.month())
                                    << "-" << std::setfill('0') << std::setw(2) << parsed_date.day();
                                birthday = oss.str();
                                std::cout << "Debug: Normalized birthday: " << birthday << std::endl;
                            }
                        } catch (const std::exception& e) {
                            std::cout << "Debug: Exception in birthday normalization: " << e.what() << ", using default" << std::endl;
                            birthday = "1970-01-01";
                        } catch (...) {
                            std::cout << "Debug: Unknown exception in birthday normalization, using default" << std::endl;
                            birthday = "1970-01-01";
                        }
                    } else {
                        // 处理非字符串类型的birthday值
                        std::stringstream ss;
                        ss << request_body["birthday"];
                        birthday = ss.str();
                    }
                } else {
                    birthday = "";
                }

                // 检查是否提供了email，这是必须的字段
                if (email.empty())
                {
                    res.code = 400;
                    nlohmann::json response;
                    response["error"] = "Email is required for user update";
                    response["success"] = false;
                    initializeCORS(req, res);
                    res.write(response.dump());
                    res.end();
                    return;
                }

                // 从数据库中获取用户信息
                // user是一个智能指针
                std::unique_ptr<User> user = nullptr;
                try
                {
                    // 获取表
                    mysqlx::Table users_table = g_database->getTable("users");

                    mysqlx::RowResult result;

                    // 直接通过email查询用户，而不是获取所有用户
                    if(!email.empty()) {
                        // 通过email查询用户
                        result = users_table.select("id", "name", "password", "CAST(phone AS CHAR)", "email", "CAST(birthday AS CHAR)", "address_id")
                                                   .where("email = :email")
                                                   .bind("email", email) // email变量的值被安全处理
                                                   .execute();
                    }
                    // 通过phone查询用户
                    else if(!phone.empty()) {
                        // 通过phone查询用户
                        result = users_table.select("id", "name", "password", "CAST(phone AS CHAR)", "email", "CAST(birthday AS CHAR)", "address_id")
                                                   .where("phone = :phone")
                                                   .bind("phone", phone) // phone变量的值被安全处理
                                                   .execute();
                    }
                    // 两个都没有就报错
                    else {
                        // 理论上不会到达这里，因为前面已经检查过了
                        res.code = 500;
                        res.set_header("Content-Type", "application/json");
                        initializeCORS(req, res);
                        res.write(R"({"error": "Either email or phone must be provided"})");
                        res.end();
                        return;
                    }
                    // 即使email变量包含恶意代码，也会被当作普通字符串值处理
                    // 处理结果
                    for (auto row : result)
                    {
                        user = std::make_unique<User>();
                        user->id = row[0].get<int>();
                        // 确保正确处理所有字段，添加错误检查
                        try {
                            user->name = clean_string(row[1].get<std::string>());
                        } catch (...) { 
                            user->name = "[Invalid Name]";
                        }
                        
                        try {
                            user->password = clean_string(row[2].get<std::string>());
                        } catch (...) {
                            user->password = "";
                        }
                        
                        try {
                            user->phone = clean_string(row[3].get<std::string>());
                        } catch (...) {
                            user->phone = "";
                        }

                        try {
                            user->email = clean_string(row[4].get<std::string>());
                        } catch (...) {
                            user->email = "";
                        }

                       // 先获取字符串格式的日期，再转换为boost::gregorian::date
                        try {
                            if (!row[5].isNull()) {
                                std::string date_str;
                        
                                // 尝试多种方式获取日期字符串
                                try {
                                    // 首先尝试直接获取字符串
                                    date_str = row[5].get<std::string>();
                                } catch (...) {
                                    try {
                                        // 如果直接获取字符串失败，尝试使用 operator<< 输出流方式
                                        // 这是处理 MySQL X DevAPI 值的推荐方法
                                        std::stringstream ss;
                                        ss << row[5];
                                        date_str = ss.str();
                                        
                                        // 清理可能的乱码字符
                                        date_str = clean_string(date_str);
                                    } catch (...) {
                                        // 如果所有方法都失败了，使用默认值
                                        date_str = "1970-01-01";
                                    }
                                }
                                
                                #ifdef DEBUG
                                std::cout << "Debug: Raw date string from database: '" << date_str << "'" << std::endl;
                                std::cout << "Debug: Raw date string length: " << date_str.length() << std::endl;
                                #endif
                        
                                // 检查是否为空或长度过短
                                if (date_str.empty() || date_str.length() < 4) {
                                    #ifdef DEBUG
                                    std::cout << "Debug: Empty or too short date string, using default" << std::endl;
                                    #endif
                                    user->birthday = boost::gregorian::date(1970, 1, 1);
                                } else {
                                    // 清理字符串，移除可能的乱码或不可见字符
                                    date_str = clean_string(date_str);
                                    
                                    // 确保日期格式正确 (YYYY-MM-DD)
                                    std::regex date_pattern(R"(^(\d{4})-(\d{1,2})-(\d{1,2})$)");
                                    std::smatch match;
                                    if (std::regex_match(date_str, match, date_pattern)) {
                                        try {
                                            // 格式化为标准的 YYYY-MM-DD 格式
                                            int year = std::stoi(match[1].str());
                                            int month = std::stoi(match[2].str());
                                            int day = std::stoi(match[3].str());
                                            
                                            // 验证日期的有效性
                                            if (year >= 1900 && year <= 2099 && 
                                                month >= 1 && month <= 12 && 
                                                day >= 1 && day <= 31) {
                                                
                                                std::ostringstream formatted_date;
                                                formatted_date << std::setfill('0') 
                                                              << std::setw(4) << year << "-"
                                                              << std::setw(2) << std::setfill('0') << month << "-"
                                                              << std::setw(2) << std::setfill('0') << day;
                                                
                                                user->birthday = boost::gregorian::from_simple_string(formatted_date.str());
                                                #ifdef DEBUG
                                                std::cout << "Debug: Parsed valid date: " << formatted_date.str() << std::endl;
                                                #endif
                                            } else {
                                                #ifdef DEBUG
                                                std::cout << "Debug: Invalid date range, using default" << std::endl;
                                                #endif
                                                user->birthday = boost::gregorian::date(1970, 1, 1);
                                            }
                                        } catch (...) {
                                            std::cout << "Debug: Failed to parse date, using default" << std::endl;
                                            user->birthday = boost::gregorian::date(1970, 1, 1);
                                        }
                                    } else {
                                        // 检查是否包含有效的日期信息
                                        if (date_str.length() >= 10) {
                                            // 尝试提取前10个字符并检查是否符合日期格式
                                            std::string potential_date = date_str.substr(0, 10);
                                            #ifdef DEBUG
                                            std::cout << "Debug: Trying potential date: " << potential_date << std::endl;
                                            #endif
                                            if (std::regex_match(potential_date, date_pattern)) {
                                                try {
                                                    user->birthday = boost::gregorian::from_simple_string(potential_date);
                                                    #ifdef DEBUG
                                                    std::cout << "Debug: Parsed potential date: " << potential_date << std::endl;
                                                    #endif
                                                } catch (...) {
                                                    #ifdef DEBUG
                                                    std::cout << "Debug: Failed to parse potential date, using default" << std::endl;
                                                    #endif
                                                    user->birthday = boost::gregorian::date(1970, 1, 1);
                                                }
                                            } else {
                                                #ifdef DEBUG
                                                std::cout << "Debug: Invalid date format, using default" << std::endl;
                                                #endif
                                                user->birthday = boost::gregorian::date(1970, 1, 1);
                                            }
                                        } else {
                                            #ifdef DEBUG
                                            std::cout << "Debug: Invalid date format, using default" << std::endl;
                                            #endif
                                            user->birthday = boost::gregorian::date(1970, 1, 1);
                                        }
                                    }
                                }
                            } else {
                                #ifdef DEBUG
                                std::cout << "Debug: Null date value, using default" << std::endl;
                                #endif
                                user->birthday = boost::gregorian::date(1970, 1, 1);
                            }
                        } catch (const std::exception& e) {
                            #ifdef DEBUG
                            std::cout << "Debug: Exception in date processing: " << e.what() << std::endl;
                            #endif
                            user->birthday = boost::gregorian::date(1970, 1, 1);
                        } catch (...) {
                            #ifdef DEBUG
                            std::cout << "Debug: Unknown exception in date processing" << std::endl;
                            #endif
                            user->birthday = boost::gregorian::date(1970, 1, 1);
                        }

                        // 处理可能为NULL的address_id
                        if (!row[6].isNull())
                        {
                            try
                            {
                                user->address_id = row[6].get<std::string>();
                            }
                            catch (...)
                            {
                                user->address_id = "1"; // 默认值
                            }
                        }
                        else
                        {
                            user->address_id = "1"; // 默认值
                        }
                        break; // 只需要第一个匹配的用户
                    }
                }
                catch (const mysqlx::Error &e)
                {
                    // 数据库操作错误
                    std::cerr << "Database error: " << e.what() << std::endl;
                    res.code = 500;
                    res.set_header("Content-Type", "application/json");
                    // 添加CORS头
                    initializeCORS(req, res);

                    res.write(R"({"error": "Database operation failed", "details": ")" + std::string(e.what()) + "\"}");
                }
                catch (const std::exception &e)
                {
                    // 其他错误
                    std::cerr << "Error: " << e.what() << std::endl;
                    res.code = 500;
                    // 添加CORS头
                    initializeCORS(req, res);

                    res.write(R"({"error": "Operation failed", "details": ")" + std::string(e.what()) + "\"}");
                }

                // 如果用户不存在
                if (!user)
                {
                    res.code = 404;
                    nlohmann::json response;
                    response["error"] = "User not found";
                    response["success"] = false;
                    initializeCORS(req, res);
                    res.write(response.dump());
                    res.end();
                    return;
                }

                // 创建数据库更新操作
                mysqlx::TableUpdate update_op = users_table.update();
                bool has_changes = false; // 添加一个标志来跟踪是否有字段需要更新

                // 只有当前端数据与数据库数据不同时才更新
                if (!name.empty() && user->name != name)
                {
                    update_op.set("name", name);
                    has_changes = true;

                }
                if (!phone.empty() && user->phone != phone)
                {
                    update_op.set("phone", phone);
                    has_changes = true;
                }
                if (!email.empty() && user->email != email)
                {
                    update_op.set("email", email);
                    has_changes = true;
                }
                if (!birthday.empty() && birthday != "\t")
                {
                    // 将字符串转换为boost::gregorian::date进行比较
                    try {
                        boost::gregorian::date birthday_date = boost::gregorian::from_simple_string(birthday);
                        if (user->birthday != birthday_date)
                        {
                            update_op.set("birthday", birthday);
                            has_changes = true;
                        }
                    } catch (const std::exception& e) {
                        // 如果日期格式无效，使用默认日期进行比较
                        boost::gregorian::date default_date(1970, 1, 1);
                        if (user->birthday != default_date)
                        {
                            update_op.set("birthday", birthday);
                            has_changes = true;
                        }
                    }
                }

                // 从请求中获取地址信息
                std::string new_address_id = "";
                std::string address_text = "";
                bool address_updated = false;
                if (request_body.find("address") != request_body.end())
                {
                    // 特别处理address字段，因为它可能是null
                    if (!request_body["address"].is_null())
                    {
                        try
                        {
                            // 处理address字段，确保它是字符串类型
                            if (request_body["address"].is_string())
                            { 
                                address_text = clean_string(request_body["address"].get<std::string>());
                            }
                            else
                            {
                                address_text = request_body["address"].dump(); // 将非字符串类型转换为字符串
                            }
                        }
                        catch (...)
                        {
                            // 如果address是null，address_text保持为空字符串
                            address_text = "";
                        }
                    }

                    // 如果有地址信息不为null，调用 geocodeAddress 函数获取地理编码
                    // 只有当地址文本非空时才进行地理编码
                    if (!address_text.empty())
                    {
                        std::string geocoded_result = geocodeAddress(address_text);
                        if (!geocoded_result.empty())
                        {
                            try
                            {
                                nlohmann::json geo_json = nlohmann::json::parse(geocoded_result);
                                if (geo_json["status"] == "1" && !geo_json["geocodes"].empty())
                                {
                                    auto &geo = geo_json["geocodes"][0];
                                    // 高德地图返回的是 "longitude,latitude" 格式的字符串
                                    std::string location_str = geo["location"].get<std::string>();
                                    // 需要分割字符串获取经纬度
                                    size_t comma_pos = location_str.find(',');
                                    double longitude = 0.0;
                                    double latitude = 0.0;
                                    if (comma_pos != std::string::npos)
                                    {
                                        longitude = std::stod(location_str.substr(0, comma_pos));
                                        latitude = std::stod(location_str.substr(comma_pos + 1));
                                        // 然后使用这些值
                                    }

                                    // 这里应该将经纬度信息保存到 address_small 表中
                                    // 并返回新创建的 address_id
                                    // 由于缺少 address_small 表的相关代码，这里简化处理
                                    // 实际应用中需要实现完整的地址管理逻辑
                                    // 保存地址到数据库并获取address_id
                                    new_address_id = saveAddressToDatabase(address_text, longitude, latitude);
                                    address_updated = true; // 标记地址已更新

                                    // 如果地址ID发生了变化，更新users表中的address_id
                                    if (new_address_id != user->address_id)
                                    {
                                        try {
                                            update_op.set("address_id", new_address_id);
                                            has_changes = true;
                                        } catch (const std::exception& e) {
                                            std::cerr << "Failed to update address_id field: " << e.what() << ", value: " << new_address_id << std::endl;
                                        }
                                    }
                                }
                            }
                            catch (const std::exception &e)
                            {
                                // 如果地理编码失败，记录错误但不中断更新过程
                                std::cerr << "Geocoding failed for address: " << address_text << ", Error: " << e.what() << std::endl;
                            }
                            catch (...)
                            {
                                // 如果地理编码失败，不更新地址
                                std::cerr << "Geocoding failed for address: " << address_text << std::endl;
                            }
                        }
                    }
                    // 处理address_id更新
                    if (request_body.find("address_id") != request_body.end() && !request_body["address_id"].is_null())
                    {
                        try
                        {
                            std::string request_address_id = request_body["address_id"].get<std::string>();
                            // 只有当address_id有效且与当前值不同时才更新
                            if (!request_address_id.empty() && request_address_id != "0" && request_address_id != user->address_id)
                            {
                                update_op.set("address_id", request_address_id);
                                has_changes = true;
                            }
                        }
                        catch (...)
                        {
                            // address_id不是有效整数，忽略
                            std::cerr << "Invalid address_id provided" << std::endl;
                        }
                    }
                    else if (address_updated && new_address_id != user->address_id)
                    {
                        // 如果通过地址文本更新了地址，则更新address_id
                        try {
                            update_op.set("address_id", new_address_id);
                            has_changes = true;
                        } catch (const std::exception& e) {
                            std::cerr << "Failed to update address_id field: " << e.what() << ", value: " << new_address_id << std::endl;
                        }
                    }
                }

                // 执行更新操作
                if (has_changes)
                {
                    try
                    {
                        update_op.where("email = :email").bind("email", email).execute();
                        std::cout << "User data updated successfully" << std::endl;
                    }
                    catch (const mysqlx::Error &e)
                    {
                        std::cerr << "Database update error: " << e.what() << std::endl;
                        res.code = 500;
                        res.set_header("Content-Type", "application/json");
                        initializeCORS(req, res);
                        res.write(R"({"error": "Failed to update user data", "details": ")" + std::string(e.what()) + "\"}");
                        res.end();
                        return;
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Standard exception during database update: " << e.what() << std::endl;
                        res.code = 500;
                        res.set_header("Content-Type", "application/json");
                        initializeCORS(req, res);
                        res.write(R"({"error": "Failed to update user data", "details": ")" + std::string(e.what()) + "\"}");
                        res.end();
                        return;
                    }
                    catch (...)
                    {
                        std::cerr << "Unknown error during database update" << std::endl;
                        res.code = 500;
                        res.set_header("Content-Type", "application/json");
                        initializeCORS(req, res);
                        res.write(R"({"error": "Failed to update user data", "details": "Unknown error"})");
                        res.end();
                        return;
                    }
                }
                else
                {
                    std::cout << "No fields to update" << std::endl;
                }
                
                // 返回成功响应
                nlohmann::json response;
                response["success"] = true;
                response["message"] = "Form data saved successfully";
                
                // 手动构建用户JSON对象，确保birthday正确序列化
                nlohmann::json user_json;
                user_json["id"] = user->id;
                user_json["name"] = user->name;
                user_json["email"] = user->email;
                user_json["phone"] = user->phone;
                user_json["address_id"] = user->address_id;
                
                // 特别处理birthday字段，将其转换为字符串格式
                std::ostringstream oss;
                oss << user->birthday;
                user_json["birthday"] = oss.str();
                
                response["user"] = user_json;
                res.code = 200;
                // 添加CORS头
                initializeCORS(req, res);

                res.write(response.dump());
            }
        }
        catch (const mysqlx::Error &e)
        {
            // 数据库操作错误
            std::cerr << "Database error: " << e.what() << std::endl;
            res.code = 500;
            res.set_header("Content-Type", "application/json");
            // 添加CORS头
            initializeCORS(req, res);

            res.write(R"({"error": "Database operation failed", "details": ")" + std::string(e.what()) + "\"}");
        }
        catch (const std::exception &e)
        {
            // 其他错误
            std::cerr << "Error: " << e.what() << std::endl;
            res.code = 500;
            // 添加CORS头
            initializeCORS(req, res);

            res.write(R"({"error": "Operation failed", "details": ")" + std::string(e.what()) + "\"}");
        }

            res.end(); });
    // 获取数据库数据的路由
    CROW_ROUTE(app, "/api/user/data")
        .methods(crow::HTTPMethod::Get)([](const crow::request &req, crow::response &res)
                                    {
        if (!g_db_session || !g_database) {
            res.code = 500;
            initializeCORS(req, res);
            res.write(R"({"error": "Database connection not available"})");
            res.end();
            return;
        }

        try {
            mysqlx::Table users_table = g_database->getTable("users");
            mysqlx::RowResult result = users_table.select("id", "name", "email", "phone", "birthday", "address_id").execute();

            nlohmann::json response_data = nlohmann::json::array();
            for (auto row : result) {
                nlohmann::json user_json;
                user_json["id"] = row[0].get<int>();
                user_json["name"] = clean_string(row[1].get<std::string>());
                user_json["email"] = clean_string(row[2].get<std::string>());
                user_json["phone"] = clean_string(row[3].get<std::string>());
                user_json["birthday"] = clean_string(row[4].get<std::string>());
                user_json["address_id"] = clean_string(row[5].get<std::string>());

                response_data.push_back(user_json);
            }

            res.code = 200;
            initializeCORS(req, res);
            res.write(response_data.dump());
        }
        catch (const std::exception& e) {
            res.code = 500;
            initializeCORS(req, res);
            res.write(R"({"error": "Failed to fetch data", "details": ")" + std::string(e.what()) + "\"}");
        }
        res.end();
    });

    CROW_WEBSOCKET_ROUTE(app, "/websocket")
        // 连接开启时的onOpen回调
        .onopen([&](crow::websocket::connection &conn)
                {
                std::lock_guard<std::mutex> lock(conn_mutex);
                active_connections.insert(&conn);
                std::cout << "New WebSocket connection opened. Total connections: " << active_connections.size() << std::endl;
                
                // 发送欢迎消息
                nlohmann::json json_msg = {{"message", "Connected to C++ WebSocket!"}};
                try {
                    conn.send_text(json_msg.dump());
                } catch (const std::exception& e) {
                    std::cerr << "Error sending welcome message: " << e.what() << std::endl;
                } })
        .onclose([&](crow::websocket::connection &conn, const std::string &reason, uint16_t value)
                 {
                     std::lock_guard<std::mutex> lock(conn_mutex);
                     active_connections.erase(&conn);
                     std::cout << "Connection closed: Code: " << value << ", Reason: " << reason
                               << ", Remaining connections: " << active_connections.size() << std::endl;
                     // 检查关闭代码
                     if (value != 1000)
                     { // 1000是正常关闭代码
                         std::cout << "Abnormal closure detected" << std::endl;
                     }

                     shutdown_cv.notify_all(); // 通知等待的线程
                 })
        .onmessage([&](crow::websocket::connection &conn, const std::string &data, bool is_binary)
                   {
                std::cout << "Message received: " << data << std::endl;
                    // 回显消息
                    if (!is_binary)
                    {
                        try {
                            nlohmann::json response = {{"echo", data}};
                            conn.send_text(response.dump());
                        } catch (const std::exception& e) {
                            std::cerr << "Error echoing message: " << e.what() << std::endl;
                        }
                    } })
        .onerror([&](crow::websocket::connection &conn, const std::string &reason)
                 { std::cerr << "WebSocket error: " << reason << std::endl; });
}

// 启动心跳线程，定期发送ping消息
void WebSocketServer::startHeartbeat()
{
    heartbeat_thread = std::thread([this]
                                   {
            while (heartbeat_running) {
                std::this_thread::sleep_for(std::chrono::seconds(30));
                std::unordered_set<crow::websocket::connection*> connections_copy;
                // 获取连接副本
                {
                    std::lock_guard<std::mutex> lock(conn_mutex);
                    connections_copy = active_connections;
                }
                // 创建可能需要移除的连接列表
                std::vector<crow::websocket::connection*> to_remove;
                for (auto* conn : connections_copy) {
                    try {
                        if (conn) {
                            conn->send_ping("ping"); // Crow支持ping/pong
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error sending ping: " << e.what() << std::endl;
                        to_remove.push_back(conn);
                    }
                }
                // 移除无效连接
                if (!to_remove.empty()) {
                    std::lock_guard<std::mutex> lock(conn_mutex);
                    for (auto* conn : to_remove) {
                        active_connections.erase(conn);
                    }
                }
            } });
}

// 设置信号处理函数
void WebSocketServer::setupSignalHandlers()
{
    // 使用更安全的 sigaction 替代 signal
    struct sigaction sa;
    sa.sa_handler = [](int)
    { instance().signal_received = true; };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}
