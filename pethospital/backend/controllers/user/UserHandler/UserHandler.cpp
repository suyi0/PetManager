#include "UserHandler.h"

// 在文件顶部添加常量定义
#define UPLOADS_DIR "/Users/yanghang/Code/PetManager/pethospital/frontend/src/assets/uploads"

// 添加获取文件MIME类型的函数
std::string getMimeType(const std::string &filepath)
{
    // 获取文件扩展名
    std::size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        return "application/octet-stream";
    }

    std::string extension = filepath.substr(dotPos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // 根据扩展名返回相应的MIME类型
    if (extension == "jpg" || extension == "jpeg")
    {
        return "image/jpeg";
    }
    else if (extension == "png")
    {
        return "image/png";
    }
    else if (extension == "gif")
    {
        return "image/gif";
    }
    else if (extension == "bmp")
    {
        return "image/bmp";
    }
    else if (extension == "webp")
    {
        return "image/webp";
    }
    else if (extension == "ico")
    {
        return "image/x-icon";
    }
    else if (extension == "svg")
    {
        return "image/svg+xml";
    }
    else if (extension == "txt")
    {
        return "text/plain";
    }
    else if (extension == "html" || extension == "htm")
    {
        return "text/html";
    }
    else if (extension == "css")
    {
        return "text/css";
    }
    else if (extension == "js")
    {
        return "application/javascript";
    }
    else if (extension == "json")
    {
        return "application/json";
    }
    else if (extension == "xml")
    {
        return "application/xml";
    }
    else if (extension == "pdf")
    {
        return "application/pdf";
    }
    else if (extension == "zip")
    {
        return "application/zip";
    }
    else if (extension == "rar")
    {
        return "application/x-rar-compressed";
    }
    else if (extension == "7z")
    {
        return "application/x-7z-compressed";
    }
    else if (extension == "mp3")
    {
        return "audio/mpeg";
    }
    else if (extension == "wav")
    {
        return "audio/wav";
    }
    else if (extension == "mp4")
    {
        return "video/mp4";
    }
    else if (extension == "avi")
    {
        return "video/x-msvideo";
    }
    else if (extension == "mov")
    {
        return "video/quicktime";
    }
    else
    {
        return "application/octet-stream";
    }
}

// 添加文件类型验证函数
bool isValidImageExtension(const std::string &extension)
{
    std::string ext_lower = extension;
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);
    return (ext_lower == "jpg" || ext_lower == "jpeg" || ext_lower == "png" ||
            ext_lower == "gif" || ext_lower == "webp");
}

// 添加生成唯一文件名的函数
std::string generateUniqueFilename(const std::string &original_filename)
{
    std::string extension = "";
    size_t dot_pos = original_filename.find_last_of('.');
    if (dot_pos != std::string::npos)
    {
        extension = original_filename.substr(dot_pos);
    }

    // 使用时间戳和随机数生成唯一文件名
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    // 生成随机数
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    std::string unique_name = "avatar_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));

    if (!extension.empty())
    {
        unique_name += extension;
    }

    return unique_name;
}

// 添加获取地址数据库最大ID的函数
int getAddressDatabaseMaxID()
{
    mysqlx::Table address_table = DatabaseManager::getInstance()->getSchema()->getTable("address_small");

    int new_id = 0; // 默认ID为0
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
        // 如果获取最大ID失败，就使用默认值0
        new_id = 0;
    }

    return new_id;
}

// 保存地址到数据库的函数
bool saveAddressToDatabase(int DBaddress_id, const std::string &address_text, double longitude = 0.0, double latitude = 0.0)
{
    try
    {
        // 获取 address_small 表
        mysqlx::Table address_table = DatabaseManager::getInstance()->getSchema()->getTable("address_small");

        if (DBaddress_id <= 0)
        {
            // 传入地址ID无效，取消存储地址信息操作
            return false;
        }
        else
        {
            // 检查该ID是否已存在
            mysqlx::RowResult result = address_table.select("id")
                                           .where("id = :id")
                                           .bind("id", DBaddress_id)
                                           .execute();

            // 如果已存在，使用这个ID更新对应地址信息
            if (result.fetchOne())
            {
                // 如果已存在，则更新该地址信息
                mysqlx::TableUpdate update_op = address_table.update();
                update_op.set("address_text", address_text)
                    .set("longitude", longitude)
                    .set("latitude", latitude)
                    .where("id = :id")
                    .bind("id", DBaddress_id)
                    .execute();
            }
            else // 不存在，直接使用该地址ID存储地址信息
            {
                // 插入新的地址记录
                mysqlx::TableInsert insert_op = address_table.insert("id", "address_text", "longitude", "latitude");
                insert_op.values(DBaddress_id, address_text, longitude, latitude).execute();
            }
            return true; // 成功
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error saving address to database: " << e.what() << std::endl;
        return false; // 失败
    }
}

// 获取路径最后的文件名
std::string getLastFileName(const std::string &url)
{
    // 查找最后一个斜杠的位置
    size_t lastSlashPos = url.find_last_of('/');

    // 如果找到了斜杠，则提取斜杠后面的部分作为文件名
    // std::string::npos是C++标准库中定义的一个常量，表示size_t类型的最大值
    if (lastSlashPos != std::string::npos)
    {
        return url.substr(lastSlashPos + 1);
    }

    // 如果没有找到斜杠，返回原字符串（或者返回空字符串）
    return url;
}

nlohmann::json UserHandler::getUserData(const int &id)
{
    mysqlx::Table user_table = DatabaseManager::getInstance()->getSchema()->getTable("users");

    mysqlx::RowResult result = user_table.select("id", "name", "password", "phone", "email", "CAST(birthday AS CHAR)", "address_id", "head_image")
                                   .where("id = :id")
                                   .bind("id", id)
                                   .execute();
    nlohmann::json user_data;
    for (auto row : result)
    {
        user_data = {
            {"id", row[0].get<int>()},
            {"name", row[1].get<std::string>()},
            {"phone", row[3].get<std::string>()},
            {"email", row[4].get<std::string>()},
            {"birthday", row[5].get<std::string>()},
            {"address_id", row[6].get<std::string>()},
            {"head_image", row[7].get<std::string>()},
        };
        break;
    }
    return user_data;
}
crow::response UserHandler::userLogin(const crow::request &req)
{
    try
    {
        // 解析请求体中的 JSON 数据
        crow::response res;
        auto jsonOpt = parseJson(req, res);
        nlohmann::json &request_body = jsonOpt.value();

        // 检查必要字段是否存在
        bool hasEmail = (request_body.find("email") != request_body.end());
        bool hasPhone = (request_body.find("phone") != request_body.end());
        bool hasPassword = (request_body.find("password") != request_body.end());

        // 检查是否提供了邮箱或电话号码，并且提供了密码
        if (!hasPassword && (!hasEmail || !hasPhone))
        {
            return ResponseHelper::error(req, "Missing email or phone and password");
        }

        std::string email = "";
        std::string phone = "";
        std::string password = "";
        int DBaddress_id = 0;

        // 安全地获取字段值
        if (hasEmail)
        {
            email = request_body["email"].is_string() ? request_body["email"].get<std::string>() : request_body["email"].dump();
        }
        if (hasPhone)
        {
            phone = request_body["phone"].is_string() ? request_body["phone"].get<std::string>() : request_body["phone"].dump();
        }
        if (hasPassword)
        {
            password = request_body["password"].is_string() ? request_body["password"].get<std::string>() : request_body["password"].dump();
        }

        // 对输入的密码进行SHA-256哈希处理
        std::string hashed_password = sha256_hash(password);

        // 检查数据库连接是否存在
        if (!dbManager || !dbManager->getSession() || !dbManager->getSchema())
        {
            std::cout << "Database connection is not available" << std::endl;
            return ResponseHelper::system_error(req);
        }

        // 从数据库中获取用户信息
        // user是一个智能指针
        std::unique_ptr<User> user = nullptr;
        try
        {
            // 获取表
            mysqlx::Table users_table = dbManager->getSchema()->getTable("users");

            // 查询用户
            mysqlx::RowResult result;
            if (!email.empty())
            {
                // 通过email查询用户
                result = users_table.select("id", "type_id", "name", "password", "phone", "email", "CAST(birthday AS CHAR)", "address_id", "head_image")
                             .where("email = :email")
                             .bind("email", email)
                             .execute();
            }
            else if (!phone.empty())
            {
                // 通过phone查询用户
                result = users_table.select("id", "type_id", "name", "password", "phone", "email", "CAST(birthday AS CHAR)", "address_id", "head_image")
                             .where("phone = :phone")
                             .bind("phone", phone)
                             .execute();
            }
            else
            {
                // 理论上不会到达这里，因为前面已经检查过了
                return crow::response(500, R"({"error": "Either email or phone must be provided"})");
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
                    user->setName(clean_string(row[2].get<std::string>()));
                }
                catch (...)
                {
                    user->setName("");
                }
                try
                {
                    user->setPassword(clean_string(row[3].get<std::string>()));
                }
                catch (...)
                {
                    user->setPassword("");
                }
                try
                {
                    user->setPhone(clean_string(row[4].get<std::string>()));
                }
                catch (...)
                {
                    user->setPhone("");
                }
                try
                {
                    user->setEmail(clean_string(row[5].get<std::string>()));
                }
                catch (...)
                {
                    user->setEmail("");
                }
                // 处理生日字段，确保其格式正确
                try
                {
                    auto birthday_value = row[6];
                    if (birthday_value.isNull())
                    {
                        user->setBirthday(boost::gregorian::date(1970, 1, 1));
                    }
                    else
                    {
                        // 获取生日字符串
                        std::string birthday_str = birthday_value.get<std::string>();

                        // 清理字符串，移除可能的空格或不可见字符
                        birthday_str = clean_string(birthday_str);

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
                    if (!row[7].isNull())
                    {
                        DBaddress_id = row[7].get<int>();
                        user->setAddressID(DBaddress_id); // 设置地址ID
                    }
                    else
                    {
                        user->setAddressID(DBaddress_id); // 默认地址ID
                    }
                }
                catch (...)
                {
                    user->setAddressID(DBaddress_id); // 默认地址ID
                }
                try
                {
                    user->setHeadImage(clean_string(row[8].get<std::string>()));
                }
                catch (...)
                {
                    user->setHeadImage("");
                }

                break; // 只需要第一个匹配的用户
            }
        }
        catch (const mysqlx::Error &e)
        {
            // 数据库操作错误
            std::cerr << "Database error: " << e.what() << std::endl;

            return ResponseHelper::custom(req, 500, "Database operation failed");
        }
        catch (const std::exception &e)
        {
            // 其他错误
            std::cerr << "Error: " << e.what() << std::endl;

            return ResponseHelper::custom(req, 500, "Operation failed");
        }

        // 在这里验证用户名和密码
        nlohmann::json response;
        if (!user || user->getPassword() != hashed_password)
        {
            // 不区分用户不存在和密码错误，统一返回相同错误信息
            return ResponseHelper::error(req, "Invalid username or password");
        }
        else
        {
            // 验证成功
            // 生成一个基于用户邮箱的JWT token
            std::string token = JwtUtils::createToken(user->getID(), user->getName(), user->getEmail(), true);
            response["token"] = token;
            response["success"] = true;

            // 手动构建用户JSON对象，确保birthday正确序列化
            nlohmann::json user_json;
            user_json["id"] = user->getID();
            user_json["type_id"] = user->getTypeID();
            user_json["name"] = user->getName();
            user_json["email"] = user->getEmail();
            user_json["phone"] = user->getPhone();
            user_json["address_id"] = user->getAddressID();
            user_json["head_image"] = user->getHeadImage();

            // 特别处理birthday字段，将其转换为字符串格式
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(4) << user->getBirthday().year()
                << "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(user->getBirthday().month())
                << "-" << std::setfill('0') << std::setw(2) << user->getBirthday().day();
            user_json["birthday"] = oss.str();

            response["user"] = user_json;
            // Return response
            return ResponseHelper::success(req, response);
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "[CRITICAL ERROR] Exception in userLogin: " << e.what() << std::endl;
        std::cout << "[CRITICAL ERROR] Stack trace would go here if available" << std::endl;
        return ResponseHelper::system_error(req, "Internal server error" + std::string(e.what()));
    }
}

crow::response UserHandler::userUpdate(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        // 获取打开对应的表
        mysqlx::Table users_table = dbManager->getSchema()->getTable("users");

        int type_id = 3;
        std::string name = "";
        std::string password = "";
        std::string phone = "";
        std::string email = "";
        std::string birthday = "";
        int address_id = 1;
        std::string headImage = "";

        int DBid = 0;
        std::string DBname = "";
        std::string DBpassword = ""; // 数据库里存储的密码就是加密后的密码（哈希值）
        std::string DBphone = "";
        std::string DBemail = "";
        boost::gregorian::date DBbirthday = boost::gregorian::date(1970, 1, 1);
        int DBaddress_id = 0;
        std::string DBheadImage = "";

        std::string hashed_password = ""; // 哈希后的密码

        // 注册时存储的数据
        if ((request_body.find("password") != request_body.end() && request_body.find("email") != request_body.end()) ||
            (request_body.find("password") != request_body.end() && request_body.find("phone") != request_body.end()))
        {
            // 创建数据库操作(插入)
            mysqlx::TableInsert insert_op = users_table.insert("type_id", "name", "phone", "password", "email", "birthday", "address_id", "head_image");

            // 从请求中获取数据并确保它们是字符串类型
            if (request_body.find("type_id") != request_body.end() && !request_body["type_id"].is_null())
            {
                type_id = request_body["type_id"].is_number() ? request_body["type_id"].get<int>() : std::stoi(std::string(request_body["type_id"]));
            }
            else
            {
                type_id = 3;
            }
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
                birthday = "1970-01-01";
            }
            if (request_body.find("address_id") != request_body.end() && !request_body["address_id"].is_null())
            {
                address_id = request_body["address_id"].is_number() ? request_body["address_id"].get<int>() : std::stoi(std::string(request_body["address_id"]));
            }
            else
            {
                // 前端没有传address_id，则自动分配一个新的数据库最大的ID
                address_id = getAddressDatabaseMaxID();
            }
            if (request_body.find("headImage") != request_body.end() && !request_body["headImage"].is_null())
            {
                headImage = request_body["headImage"].is_string() ? request_body["headImage"].get<std::string>() : request_body["headImage"].dump();
            }
            else
            {
                headImage = "";
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

            // 使用SHA-256对密码进行哈希处理
            std::string hashed_password = sha256_hash(password);

            insert_op.values(type_id, name, phone, hashed_password, email, birthday, address_id, headImage).execute();
            // 用于执行 INSERT 操作并将数据插入到数据库.

            return ResponseHelper::success(req, "用户注册成功");
        }
        // 更新用户数据
        else if (
            (request_body.contains("name") ||
             request_body.contains("birthday") ||
             request_body.contains("address_id") ||
             request_body.contains("headImage")) &&
            (request_body.contains("email") || request_body.contains("phone")))
        {
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
            if (request_body.find("password") != request_body.end() && !request_body["password"].is_null())
            {
                if (request_body["password"].is_string())
                {
                    password = request_body["password"].get<std::string>();
                }
                else
                {
                    password = request_body["password"].dump();
                }
                hashed_password = sha256_hash(password);
            }
            else
            {
                hashed_password = "";
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
            if (request_body.find("headImage") != request_body.end() && !request_body["headImage"].is_null())
            {
                if (request_body["headImage"].is_string())
                {
                    headImage = request_body["headImage"].get<std::string>();
                }
                else
                {
                    headImage = request_body["headImage"].dump();
                }
            }
            else
            {
                headImage = "";
            }
            // 在保存表单数据的路由中处理生日字段
            if (request_body.find("birthday") != request_body.end() && !request_body["birthday"].is_null())
            {
                if (request_body["birthday"].is_string())
                {
                    birthday = request_body["birthday"].get<std::string>();

                    // 标准化日期格式
                    try
                    {
                        boost::gregorian::date parsed_date;

                        // 尝试解析不同格式的日期
                        if (birthday.length() >= 8)
                        { // 至少像 2003-9-20 这样的格式
                            // 检查是否已经是标准的 YYYY-MM-DD 格式
                            if (birthday.length() == 10 && birthday[4] == '-' && birthday[7] == '-')
                            {
                                parsed_date = boost::gregorian::from_simple_string(birthday);
                            }
                            // 检查是否是 YYYY-M-D 格式 (没有前导零)
                            else if (std::regex_match(birthday, std::regex(R"(^\d{4}-\d{1,2}-\d{1,2}$)")))
                            {
                                parsed_date = boost::gregorian::from_simple_string(birthday);
                            }
                            // 检查是否是 YYYY-MMM-DD 格式 (如 2003-Sep-20)
                            else if (std::regex_match(birthday, std::regex(R"(^\d{4}-[A-Za-z]{3}-\d{1,2}$)")))
                            {
                                std::istringstream iss(birthday);
                                iss.imbue(std::locale(std::locale::classic(),
                                                      new boost::gregorian::date_input_facet("%Y-%b-%d")));
                                iss >> parsed_date;
                            }
                            // 其他情况尝试直接解析
                            else
                            {
                                parsed_date = boost::gregorian::from_simple_string(birthday);
                            }

                            // 转换回标准的 YYYY-MM-DD 格式
                            std::ostringstream oss;
                            oss << std::setfill('0') << std::setw(4) << parsed_date.year()
                                << "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(parsed_date.month())
                                << "-" << std::setfill('0') << std::setw(2) << parsed_date.day();
                            birthday = oss.str();
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cout << "Debug: Exception in birthday normalization: " << e.what() << ", using default" << std::endl;
                        birthday = "1970-01-01";
                    }
                }
                else
                {
                    // 处理非字符串类型的birthday值
                    std::stringstream ss;
                    ss << request_body["birthday"];
                    birthday = ss.str();
                }
            }

            // 检查是否提供了email,phone，这是必须的字段
            if (email.empty() || phone.empty())
            {
                return ResponseHelper::error(req, "Email and phone are required for user update");
            }

            // 从数据库中获取用户信息
            try
            {
                // 获取表
                mysqlx::Table users_table = dbManager->getSchema()->getTable("users");

                mysqlx::RowResult result;

                // 直接通过email查询用户，而不是获取所有用户
                if (!email.empty())
                {
                    // 通过email查询用户
                    result = users_table.select("id", "name", "password", "phone", "email", "CAST(birthday AS CHAR)", "address_id", "head_image")
                                 .where("email = :email")
                                 .bind("email", email) // email变量的值被安全处理
                                 .execute();
                }
                // 通过phone查询用户
                else if (!phone.empty())
                {
                    // 通过phone查询用户
                    result = users_table.select("id", "name", "password", "phone", "email", "CAST(birthday AS CHAR)", "address_id", "head_image")
                                 .where("phone = :phone")
                                 .bind("phone", phone) // phone变量的值被安全处理
                                 .execute();
                }

                if (result.count() == 0)
                {
                    return ResponseHelper::notFound(req, "User not found");
                }

                // 即使email变量包含恶意代码，也会被当作普通字符串值处理
                // 处理结果,把数据库数据放入user对象中
                for (auto row : result)
                {
                    DBid = row[0].get<int>();
                    // 确保正确处理所有字段，添加错误检查
                    try
                    {
                        DBname = clean_string(row[1].get<std::string>());
                    }
                    catch (...)
                    {
                        DBname = "Invalid Name";
                    }

                    try
                    {
                        DBpassword = clean_string(row[2].get<std::string>());
                    }
                    catch (...)
                    {
                        DBpassword = "Invalid Password";
                    }

                    try
                    {
                        DBphone = clean_string(row[3].get<std::string>());
                    }
                    catch (...)
                    {
                        DBphone = "Invalid Phone";
                    }

                    try
                    {
                        DBemail = clean_string(row[4].get<std::string>());
                    }
                    catch (...)
                    {
                        DBemail = "Invalid Email";
                    }

                    // 先获取字符串格式的日期，再转换为boost::gregorian::date
                    try
                    {
                        if (!row[5].isNull())
                        {
                            std::string date_str;

                            // 尝试多种方式获取日期字符串
                            try
                            {
                                // 首先尝试直接获取字符串
                                date_str = row[5].get<std::string>();
                            }
                            catch (...)
                            {
                                try
                                {
                                    // 如果直接获取字符串失败，尝试使用 operator<< 输出流方式
                                    // 这是处理 MySQL X DevAPI 值的推荐方法
                                    std::stringstream ss;
                                    ss << row[5];
                                    date_str = ss.str();

                                    // 清理可能的乱码字符
                                    date_str = clean_string(date_str);
                                }
                                catch (...)
                                {
                                    // 如果所有方法都失败了，使用默认值
                                    date_str = "1970-01-01";
                                }
                            }

#ifdef DEBUG
                            std::cout << "Debug: Raw date string from database: '" << date_str << "'" << std::endl;
                            std::cout << "Debug: Raw date string length: " << date_str.length() << std::endl;
#endif

                            // 检查是否为空或长度过短
                            if (date_str.empty() || date_str.length() < 4)
                            {
#ifdef DEBUG
                                std::cout << "Debug: Empty or too short date string, using default" << std::endl;
#endif
                            }
                            else
                            {
                                // 清理字符串，移除可能的乱码或不可见字符
                                date_str = clean_string(date_str);

                                // 确保日期格式正确 (YYYY-MM-DD)
                                std::regex date_pattern(R"(^(\d{4})-(\d{1,2})-(\d{1,2})$)");
                                std::smatch match;
                                if (std::regex_match(date_str, match, date_pattern))
                                {
                                    try
                                    {
                                        // 格式化为标准的 YYYY-MM-DD 格式
                                        int year = std::stoi(match[1].str());
                                        int month = std::stoi(match[2].str());
                                        int day = std::stoi(match[3].str());

                                        // 验证日期的有效性
                                        if (year >= 1900 && year <= 2099 &&
                                            month >= 1 && month <= 12 &&
                                            day >= 1 && day <= 31)
                                        {

                                            std::ostringstream formatted_date;
                                            formatted_date << std::setfill('0')
                                                           << std::setw(4) << year << "-"
                                                           << std::setw(2) << std::setfill('0') << month << "-"
                                                           << std::setw(2) << std::setfill('0') << day;

                                            DBbirthday = boost::gregorian::from_simple_string(formatted_date.str());
#ifdef DEBUG
                                            std::cout << "Debug: Parsed valid date: " << formatted_date.str() << std::endl;
#endif
                                        }
                                        else
                                        {
#ifdef DEBUG
                                            std::cout << "Debug: Invalid date range, using default" << std::endl;
#endif
                                        }
                                    }
                                    catch (...)
                                    {
                                        std::cout << "Debug: Failed to parse date, using default" << std::endl;
                                    }
                                }
                                else
                                {
                                    // 检查是否包含有效的日期信息
                                    if (date_str.length() >= 10)
                                    {
                                        // 尝试提取前10个字符并检查是否符合日期格式
                                        std::string potential_date = date_str.substr(0, 10);
#ifdef DEBUG
                                        std::cout << "Debug: Trying potential date: " << potential_date << std::endl;
#endif
                                        if (std::regex_match(potential_date, date_pattern))
                                        {
                                            try
                                            {
                                                DBbirthday = boost::gregorian::from_simple_string(potential_date);
#ifdef DEBUG
                                                std::cout << "Debug: Parsed potential date: " << potential_date << std::endl;
#endif
                                            }
                                            catch (...)
                                            {
#ifdef DEBUG
                                                std::cout << "Debug: Failed to parse potential date, using default" << std::endl;
#endif
                                            }
                                        }
                                        else
                                        {
#ifdef DEBUG
                                            std::cout << "Debug: Invalid date format, using default" << std::endl;
#endif
                                        }
                                    }
                                    else
                                    {
#ifdef DEBUG
                                        std::cout << "Debug: Invalid date format, using default" << std::endl;
#endif
                                    }
                                }
                            }
                        }
                        else
                        {
#ifdef DEBUG
                            std::cout << "Debug: Null date value, using default" << std::endl;
#endif
                        }
                    }
                    catch (const std::exception &e)
                    {
#ifdef DEBUG
                        std::cout << "Debug: Exception in date processing: " << e.what() << std::endl;
#endif
                    }
                    catch (...)
                    {
#ifdef DEBUG
                        std::cout << "Debug: Unknown exception in date processing" << std::endl;
#endif
                    }

                    // 处理可能为NULL的address_id
                    if (!row[6].isNull())
                    {
                        DBaddress_id = row[6].get<int>();
                    }
                    else
                    {
                        DBaddress_id = 0;
                    }
                    try
                    {
                        DBheadImage = row[7].get<std::string>();
                    }
                    catch (...)
                    {
                        DBheadImage = "";
                    }
                    break; // 只需要第一个匹配的用户
                }
            }
            catch (const mysqlx::Error &e)
            {
                // 数据库操作错误
                return ResponseHelper::system_error(req, "Database error: " + std::string(e.what()) + "\"");
            }
            catch (const std::exception &e)
            {
                // 其他错误
                return ResponseHelper::system_error(req, "Error: " + std::string(e.what()) + "\"");
            }

            // 创建数据库更新操作
            mysqlx::TableUpdate update_op = users_table.update();
            bool has_changes = false; // 添加一个标志来跟踪是否有字段需要更新

            // 只有当前端数据与数据库数据不同时才更新
            if (!name.empty() && DBname != name)
            {
                update_op.set("name", name);
                has_changes = true;
            }
            // 对比哈希密码，值不相同才更新
            if (!hashed_password.empty() && DBpassword != hashed_password)
            {
                update_op.set("password", hashed_password);
                has_changes = true;
            }
            if (!phone.empty() && DBphone != phone)
            {

                update_op.set("phone", phone);
                has_changes = true;
            }
            if (!email.empty() && DBemail != email)
            {
                update_op.set("email", email);
                has_changes = true;
            }
            if (!birthday.empty() && birthday != "\t" && birthday != "1970-01-01")
            {
                // 将字符串转换为boost::gregorian::date进行比较
                try
                {
                    boost::gregorian::date birthday_date = boost::gregorian::from_simple_string(birthday);
                    if (DBbirthday != birthday_date)
                    {
                        update_op.set("birthday", birthday);
                        has_changes = true;
                    }
                }
                catch (const std::exception &e)
                {
                    // 如果日期格式无效，使用默认日期进行比较
                    boost::gregorian::date default_date(1970, 1, 1);
                    if (DBbirthday != default_date)
                    {
                        update_op.set("birthday", birthday);
                        has_changes = true;
                    }
                }
            }
            if (!headImage.empty() && DBheadImage != headImage)
            {
                // 删除原来的图片，如果文件不存在也不会报错
                const std::string lastFileName = getLastFileName(DBheadImage);
                std::string oldFilePath = std::string(UPLOADS_DIR) + "/" + lastFileName;

                // 检查文件是否存在后再删除，避免删除目录
                if (std::filesystem::exists(oldFilePath) && !std::filesystem::is_directory(oldFilePath))
                {
                    try
                    {
                        std::filesystem::remove(oldFilePath);
                        std::cout << "Successfully removed old avatar file: " << oldFilePath << std::endl;
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error removing old avatar file: " << e.what() << std::endl;
                        // 不中断更新过程，仅记录错误
                    }
                }
                else
                {
                    std::cout << "Old avatar file does not exist or is a directory: " << oldFilePath << std::endl;
                }

                update_op.set("head_image", headImage);
                has_changes = true;
            }
            // 从请求中获取地址信息
            std::string address_text = "";
            if (request_body.find("address") != request_body.end()) // 只有前端传入address字段时才进行更新地址信息操作
            {
                // 特别处理address字段，因为它可能是null
                if (!request_body["address"].is_null()) // address字段不为null
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
                else // address字段为null
                {
                    // 如果address是null，address_text保持为空字符串
                    address_text = "";
                }

                // 如果有地址信息不为null，调用 geocodeAddress 函数获取地理编码
                // 只有当地址文本非空时才进行地理编码,并更新地址信息
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
                                if (saveAddressToDatabase(DBaddress_id, address_text, longitude, latitude))
                                {
                                    std::cout << "Address saved to database successfully" << std::endl;
                                }
                                else
                                {
                                    std::cerr << "Error saving address to database" << std::endl;
                                }
                            }
                        }
                        catch (const std::exception &e)
                        {
                            // 如果地理编码失败，也记录地址错误到数据库，但经纬度设为0.00
                            saveAddressToDatabase(DBaddress_id, "Geocoding failed for address: " + address_text, 0.0, 0.0);
                            std::cerr << "Failed to save geocoding error address to database" << std::endl;
                        }
                    }
                }
            }

            // 执行更新操作
            if (has_changes)
            {
                try
                {
                    // 根据id进行更新
                    if (DBid != 0)
                    {
                        update_op.where("id = :id").bind("id", DBid).execute();
                    }
                    std::cout << "User data updated successfully" << std::endl;
                }
                catch (const mysqlx::Error &e)
                {
                    std::cerr << "Database update error: " << e.what() << std::endl;

                    return ResponseHelper::custom(req, 500, "error: Failed to update user data, details: " + std::string(e.what()) + "\"");
                }
                // 返回成功响应
                nlohmann::json response;
                response["success"] = true;
                response["message"] = "Form data saved successfully";
                response["data"] = getUserData(DBid);

                return ResponseHelper::success(req, response);
            }
            else
            {
                return ResponseHelper::success(req, "No changes to update");
            }
        }
        else
        {
            // 请求不符合注册或更新条件
            return ResponseHelper::error(req, "responNo valid action performed - request does not contain required fields for registration or update");
        }
    }
    catch (const mysqlx::Error &e)
    {
        // 数据库连接错误
        std::cerr << "Database connection error: " << e.what() << std::endl;

        return ResponseHelper::custom(req, 500, "error: Database connection failed, details: " + std::string(e.what()) + "\"");
    }
    catch (const std::exception &e)
    {
        // 其他错误
        std::cerr << "Error: " << e.what() << std::endl;

        return ResponseHelper::custom(req, 500, "error: " + std::string(e.what()));
    }
}

crow::response UserHandler::userUploadAvatar(const crow::request &req)
{
    try
    {
        // 检查数据库连接是否存在
        if (!checkDbConnection())
        {
            return ResponseHelper::system_error(req);
        }

        // 获取multipart数据
        crow::multipart::message msg(req);
        auto &parts = msg.parts;

        std::string filename = "";        // 文件名
        std::string filepath = "";        // 保存文件路径
        std::string file_extension = "";  // 文件扩展名
        std::string unique_filename = ""; // 用于保存生成的唯一文件名

        for (size_t i = 0; i < parts.size(); ++i)
        {
            auto &part = parts[i];

            // 获取Content-Disposition头部信息
            auto header_obj = part.get_header_object("Content-Disposition");
            std::string part_name = "";
            std::string part_filename = "";

            if (header_obj.params.find("name") != header_obj.params.end())
            {
                part_name = header_obj.params.at("name");
            }

            if (header_obj.params.find("filename") != header_obj.params.end())
            {
                part_filename = header_obj.params.at("filename");
            }

            // 检查是否是文件字段
            if (part_name == "image" || part_name == "avatar")
            {
                filename = part_filename;

                // 提取文件扩展名
                size_t dot_pos = filename.find_last_of('.');
                if (dot_pos != std::string::npos)
                {
                    file_extension = filename.substr(dot_pos + 1);
                }

                // 验证文件类型
                if (!isValidImageExtension(file_extension))
                {
                    return ResponseHelper::custom(req, 400, "error: Invalid file type. Only image files are allowed.");
                }

                // 生成唯一文件名
                unique_filename = generateUniqueFilename(filename);

                // 检查文件是否存在
                if (!std::filesystem::exists(UPLOADS_DIR))
                {
                    // 创建目录
                    std::filesystem::create_directories(UPLOADS_DIR);
                }

                filepath = std::string(UPLOADS_DIR) + "/" + unique_filename;

                // 保存文件
                std::ofstream file(filepath, std::ios::binary);
                if (file.is_open())
                {
                    file << part.body;
                    file.close();
                }
                else
                {
                    return ResponseHelper::custom(req, 500, "Failed to save file");
                }
            }
        }

        // 构建响应
        std::string avatar_url = "http://localhost:8081/uploads/" + unique_filename;
        nlohmann::json response;
        response["success"] = true;
        response["message"] = "File uploaded successfully";
        response["avatarUrl"] = avatar_url;
        response["filename"] = unique_filename;

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in file upload: " << e.what() << std::endl;

        return ResponseHelper::system_error(req);
    }
}

crow::response UserHandler::upload(const crow::request &req, const std::string &filename)
{
    crow::response res;

    // 使用项目目录中的文件路径
    std::string filepath = std::string(UPLOADS_DIR) + "/" + filename;

    // 检查文件是否存在
    if (!std::filesystem::exists(filepath))
    {
        std::cerr << "File not found: " << filepath << std::endl;
        res.code = 404;
        res.end();
        return res;
    }

    // 检查是否为目录
    if (std::filesystem::is_directory(filepath))
    {
        res.code = 403;
        res.end();
        return res;
    }

    // 设置响应头
    res.set_header("Content-Type", getMimeType(filepath));
    res.set_header("Cache-Control", "public, max-age=3600");

    // 读取文件内容
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        res.code = 500;
        res.end();
        return res;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    res.body = buffer.str();

    res.end();
    return res;
}

// 获取用户数据
crow::response UserHandler::getData(const crow::request &req)
{
    crow::response res;
    // auto request_body_opt = validateRequest(req, res);
    // if (!request_body_opt)
    //     return res;
    // auto &request_body = request_body_opt.value();

    try
    {
        mysqlx::Table users_table = dbManager->getSchema()->getTable("users");
        mysqlx::RowResult result = users_table.select("id", "name", "phone", "email", "CAST(birthday AS CHAR)", "address_id", "head_image").execute();

        nlohmann::json response_data = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json user_json;
            user_json["id"] = row[0].get<int>();
            user_json["name"] = clean_string(row[1].get<std::string>());
            user_json["phone"] = clean_string(row[2].get<std::string>());
            user_json["email"] = clean_string(row[3].get<std::string>());
            user_json["birthday"] = clean_string(row[4].get<std::string>());
            user_json["address_id"] = clean_string(row[5].get<std::string>());
            user_json["head_image"] = clean_string(row[6].get<std::string>());

            response_data.push_back(user_json);
        }

        return ResponseHelper::success(req, response_data);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::custom(req, 500, "error: Failed to fetch data, details: " + std::string(e.what()) + "\"");
    }
}