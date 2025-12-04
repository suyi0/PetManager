#include "ReservationRoutes.h"

// 实现WebSocketServer的单例模式instance方法
ReservationRoutes &ReservationRoutes::instance()
{
    static ReservationRoutes instance;
    return instance;
}
void ReservationRoutes::setupReservationRoutes(crow::SimpleApp &app)
{
    // 添加标志防止重复设置路由
    static bool routes_setup = false;
    if (routes_setup)
        return;
    routes_setup = true;

    //  预约时间表生成路由
    CROW_ROUTE(app, "/api/reservate/schedule")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([](const crow::request &req, crow::response &res)
                                                                    {
            try {
                Reservate r;
                
                // 生成并返回时间表
                auto schedule = r.generateSchedule();

                // 包装成包含 success 字段的 JSON 对象
                nlohmann::json response;
                response["success"] = true;
                response["data"] = schedule;
        
                res.code = 200;
                res.set_header("Content-Type", "application/json");
                res.write(response.dump());
            } catch (const std::exception& e) {
                res.code = 500;
                res.write(R"({"error": "Failed to generate schedule"})");
            }
            res.end(); });

    // 存储预约记录
    CROW_ROUTE(app, "/api/reservate/record")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([](const crow::request &req, crow::response &res)
                                                                    { 
            try {
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
                bool hasName = (request_body.find("name") != request_body.end());
                bool hasEmail = (request_body.find("email") != request_body.end());
                bool hasPhone = (request_body.find("phone") != request_body.end());
                if (!hasName || !hasEmail || !hasPhone)
                {
                    res.code = 400;
                    initializeCORS(req,res);
                    res.write(R"({"error": "Missing required fields"})");
                    res.end(); // 显式结束响应
                    return;
                }

                std::string name = "";
                std::string email = "";
                std::string phone = "";

                // 安全获取字段信息
                if(hasName)
                {
                    name = request_body["name"].is_string() ? 
                            request_body["name"].get<std::string>() : 
                            request_body["name"].dump();
                }

                if(hasEmail)
                {
                    email = request_body["email"].is_string() ? 
                            request_body["email"].get<std::string>() : 
                            request_body["email"].dump();
                }

                if(hasPhone)
                {
                    phone = request_body["phone"].is_string() ? 
                            request_body["phone"].get<std::string>() : 
                            request_body["phone"].dump();
                }

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
                int user_id = 0;
                int doctor_id = 0;
                std::string date = "";
                std::string time_slot = "";
                std::string status = "预约成功";
                std::string creation_time = "";
                try {
                    // 获取表
                    mysqlx::Table users_table = g_database->getTable("users");
                    mysqlx::Table reaservation_table = g_database->getTable("reaservations");

                    mysqlx::RowResult users_result;
                    if(!email.empty()) {
                        users_result = users_table.select("id")
                                            .where("email = :email")
                                            .bind("email", email)
                                            .execute();
                    } else if(!phone.empty()) {
                        users_result = users_table.select("id")
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

                    auto user_row = users_result.fetchOne();
                    // 如果用户不存在，返回错误
                    if (!user_row) {
                        res.code = 404;
                        res.set_header("Content-Type", "application/json");
                        initializeCORS(req, res);
                        res.write(R"({"error": "User not found"})");
                        res.end();
                        return;
                    }
                    user_id = user_row[0].get<int>();
                    
                    // 创建数据库更新操作
                    // 创建数据库操作
                    mysqlx::TableInsert insert_op = reaservation_table.insert("user_id","doctor_id","date","time_slot","status","creation_time");

                    // 安全获取预约信息字段
                    // 获取并转换 doctor_id, date, time_slot 字段
                    if(request_body.find("doctor_id") != request_body.end() && !request_body["doctor_id"].is_null())
                    {
                        doctor_id = request_body["doctor_id"].is_number() ? 
                                request_body["doctor_id"].get<int>() : 
                                std::stoi(request_body["doctor_id"].dump());
                    }
                    if(request_body.find("date") != request_body.end() && !request_body["date"].is_null())
                    {
                        date = request_body["date"].is_string() ? 
                                request_body["date"].get<std::string>() : 
                                request_body["date"].dump();
                    }
                    if(request_body.find("slot") != request_body.end() && !request_body["slot"].is_null())
                    {
                        time_slot = request_body["slot"].is_string() ? 
                                request_body["slot"].get<std::string>() : 
                                request_body["slot"].dump();
                    }
                    // 获得预约记录创建时间
                    creation_time = getCreateTime();
                    // 插入数据库
                    try {
                        insert_op.values(user_id, doctor_id, date, time_slot, status, creation_time).execute();
                    } catch (const mysqlx::Error& e) {
                        std::cerr << "Database error: " << e.what() << std::endl;
                        res.code = 500;
                        initializeCORS(req,res);
                        res.write(R"({"error": "Database operation failed", "details": ")" + std::string(e.what()) + "\"}");
                        res.end();
                        return;
                    }
    
                    // 返回成功响应
                    nlohmann::json response;
                    response["success"] = true;
                    response["message"] = "预约成功";
                    res.code = 200;
                    initializeCORS(req, res);
                    res.write(response.dump());
                    res.end();
                    return;

                } catch (const std::exception& e) {
                    res.code = 500;
                    initializeCORS(req,res);
                    res.write(R"({"error": "Failed to fetch user information"})");
                    res.end();
                    return;
                }
            } catch (const std::exception& e) {
                res.code = 500;
                initializeCORS(req, res);  // 添加 CORS 头
                res.write(R"({"error": "Failed to save reservation"})");
                res.end();  // 显式结束响应
            } });

    // 取消预约记录路由
    CROW_ROUTE(app, "/api/reservate/cancel/<int>")
        .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)([](const crow::request &req, crow::response &res, int reservation_id)
                                                                    {
            try {
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

                // 检查数据库连接是否存在
                if (!g_db_session || !g_database) {
                    res.code = 500;
                    initializeCORS(req,res);
                    res.write(R"({"error": "Database connection not available"})");
                    res.end();
                    return;
                }

                std::string status = "已取消";

                // 创建数据库操作
                mysqlx::Table reaservation_table = g_database->getTable("reservations");

                mysqlx::TableUpdate update_op = reaservation_table.update();
                update_op.set("status", status).where("id = :id").bind("id", reservation_id).execute();

                // 返回成功响应
                nlohmann::json response;
                response["success"] = true;
                response["message"] = "取消成功";
                res.code = 200;
                initializeCORS(req, res);
                res.write(response.dump());
                res.end();
                return;

            } catch (const std::exception& e) {
                res.code = 500;
                initializeCORS(req,res);
                res.write(R"({"error": "Failed to cancel reservation"})");
                res.end();
                return;
            } });

    // 获得订单记录
    CROW_ROUTE(app, "/api/order/getrecord")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([](const crow::request &req, crow::response &res) {
            try {
                // 处理OPTIONS预检请求
                initializeOPTIONS(req, res);

                // 检查数据库连接是否存在
                if (!g_db_session || !g_database) {
                    res.code = 500;
                    initializeCORS(req, res);
                    res.write(R"({"error": "Database connection not available"})");
                    res.end();
                    return;
                }
            } catch (const std::exception& e) {
                res.code = 500;
                initializeCORS(req, res);
                res.write(R"({"error": "Failed to check database connection"})");
            }
        });

    // 获取预约记录列表路由
    CROW_ROUTE(app, "/api/reservate/getrecord")
        .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)([](const crow::request &req, crow::response &res)
                                                                   {
            try {
                // 处理OPTIONS预检请求
                initializeOPTIONS(req, res);

                // 检查数据库连接是否存在
                if (!g_db_session || !g_database) {
                    res.code = 500;
                    initializeCORS(req, res);
                    res.write(R"({"error": "Database connection not available"})");
                    res.end();
                    return;
                }

                try {
                    // 获取预约表
                    mysqlx::Table reaservation_table = g_database->getTable("reaservations");
                    
                    // 查询所有预约记录
                    mysqlx::RowResult result = reaservation_table.select("id", "user_id", "doctor_id", "date", "time_slot", "status", "creation_time")
                                                .orderBy("date DESC")
                                                .execute();
                    
                    // 构建响应数据
                    nlohmann::json response_data = nlohmann::json::array();
                    for (auto row : result) {
                        nlohmann::json record;
                        record["id"] = row[0].get<int>();
                        record["user_id"] = row[1].get<int>();
                        record["doctor_id"] = row[2].get<int>();
                        record["date"] = row[3].get<std::string>();
                        record["time_slot"] = row[4].get<std::string>();
                        record["status"] = row[5].get<std::string>();
                        record["creation_time"] = row[6].get<std::string>();
                        response_data.push_back(record);
                    }
                    
                    // 返回成功响应
                    nlohmann::json response;
                    response["success"] = true;
                    response["data"] = response_data;
                    res.code = 200;
                    initializeCORS(req, res);
                    res.write(response.dump());
                } catch (const mysqlx::Error& e) {
                    std::cerr << "Database error: " << e.what() << std::endl;
                    res.code = 500;
                    initializeCORS(req, res);
                    res.write(R"({"error": "Database operation failed", "details": ")" + std::string(e.what()) + "\"}");
                }
            } catch (const std::exception& e) {
                res.code = 500;
                initializeCORS(req, res);
                res.write(R"({"error": "Failed to fetch reservations", "details": ")" + std::string(e.what()) + "\"}");
            }
            res.end(); });

    // 删除预约记录路由
    CROW_ROUTE(app, "/api/reservate/record/<int>")
        .methods(crow::HTTPMethod::Delete, crow::HTTPMethod::Options)([](const crow::request &req, crow::response &res, int record_id)
                                                                      {
            try {
                // 处理OPTIONS预检请求
                initializeOPTIONS(req, res);

                // 检查数据库连接是否存在
                if (!g_db_session || !g_database) {
                    res.code = 500;
                    initializeCORS(req, res);
                    res.write(R"({"error": "Database connection not available"})");
                    res.end();
                    return;
                }

                try {
                    // 获取预约表
                    mysqlx::Table reaservation_table = g_database->getTable("reaservations");

                    // 删除指定ID的预约记录
                    mysqlx::Result result = reaservation_table.remove()
                                                .where("id = :id")
                                                .bind("id", record_id)
                                                .execute();
                    
                    // 检查是否有记录被删除
                    if (result.getAffectedItemsCount() > 0) {
                        // 返回成功响应
                        nlohmann::json response;
                        response["success"] = true;
                        response["message"] = "预约记录删除成功";
                        res.code = 200;
                        initializeCORS(req, res);
                        res.write(response.dump());
                    } else {
                        // 没有找到对应的记录
                        nlohmann::json response;
                        response["success"] = false;
                        response["error"] = "未找到指定的预约记录";
                        res.code = 404;
                        initializeCORS(req, res);
                        res.write(response.dump());
                    }
                } catch (const mysqlx::Error& e) {
                    std::cerr << "Database error: " << e.what() << std::endl;
                    res.code = 500;
                    initializeCORS(req, res);
                    res.write(R"({"error": "Database operation failed", "details": ")" + std::string(e.what()) + "\"}");
                }
            } catch (const std::exception& e) {
                res.code = 500;
                initializeCORS(req, res);
                res.write(R"({"error": "Failed to delete reservation", "details": ")" + std::string(e.what()) + "\"}");
            }
            res.end(); });
}