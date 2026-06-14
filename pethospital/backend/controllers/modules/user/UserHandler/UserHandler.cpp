#include "userHandler.h"
#include "../../../../utils/AuthIdentifierUtils.h"
#include "../userPhoneSync/userPhoneSync.h"
#include "../../../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "roleTypeUtils/roleTypeUtils.h"
#include <vector>

// 在文件顶部添加常量定义
#define UPLOADS_DIR "/Users/yanghang/Code/PetManager/pethospital/frontend/src/assets/uploads"

namespace
{
    void geocode(const std::string &address_text, double &longitude, double &latitude, std::string &geocode_source)
    {
        std::string geocoded_result = geocodeAddress(address_text);
        if (!geocoded_result.empty())
        {
            try
            {
                // 解析地理编码结果
                nlohmann::json geo_json = nlohmann::json::parse(geocoded_result);
                if (geo_json.value("status", "") == "1" &&
                    geo_json.contains("geocodes") &&
                    geo_json["geocodes"].is_array() &&
                    !geo_json["geocodes"].empty())
                {
                    auto &geo = geo_json["geocodes"][0]; // 获取JSON数组的geocodes的第一个结果
                    std::string location_str = geo.value("location", "");
                    size_t comma_pos = location_str.find(',');
                    if (comma_pos != std::string::npos)
                    {
                        longitude = std::stod(location_str.substr(0, comma_pos));
                        latitude = std::stod(location_str.substr(comma_pos + 1));
                        geocode_source = geo.value("geocode_source", "");
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to parse geocode result: " << e.what() << std::endl;
            }
        }
    }

    nlohmann::json splitUserName(const std::string &rawName)
    {
        const std::string delimiter = "·";
        std::vector<std::string> parts;
        std::string remaining = clean_string(rawName);
        std::size_t position = 0;

        while ((position = remaining.find(delimiter)) != std::string::npos)
        {
            parts.push_back(clean_string(remaining.substr(0, position)));
            remaining.erase(0, position + delimiter.length());
        }
        parts.push_back(clean_string(remaining));

        const std::string lastName = parts.size() > 0 ? parts[0] : "";
        const std::string middleName = parts.size() > 2 ? parts[1] : "";
        const std::string firstName = parts.size() > 2 ? parts[2] : (parts.size() > 1 ? parts[1] : "");

        return {
            {"lastName", lastName},
            {"middleName", middleName},
            {"firstName", firstName},
        };
    }

    // 将用户名拆分为姓、名和中间名，并添加到目标JSON对象中
    void appendUserNameParts(nlohmann::json &target, const std::string &rawName)
    {
        const nlohmann::json nameParts = splitUserName(rawName);
        target["lastName"] = nameParts["lastName"];
        target["middleName"] = nameParts["middleName"];
        target["firstName"] = nameParts["firstName"];
    }

}

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

nlohmann::json userHandler::getUserData(const int &id)
{
    if (!checkDbConnection())
    {
        return nlohmann::json::object();
    }

    mysqlx::SqlResult result = dbManager->getSession()
                                   ->sql("SELECT id, name, password, phone, email, CAST(birthday AS CHAR), head_image "
                                         "FROM users WHERE id = ?")
                                   .bind(id)
                                   .execute();
    nlohmann::json user_data;
    for (auto row : result)
    {
        const auto safeString = [&row](int index, bool cleanValue = true) -> std::string
        {
            if (row[index].isNull())
            {
                return "";
            }

            try
            {
                const std::string value = row[index].get<std::string>();
                return cleanValue ? clean_string(value) : value;
            }
            catch (...)
            {
                std::stringstream ss;
                ss << row[index];
                const std::string value = ss.str();
                return cleanValue ? clean_string(value) : value;
            }
        };

        const std::string userName = safeString(1);
        user_data = {
            {"id", row[0].get<int>()},
            {"name", userName},
            {"phone", safeString(3)},
            {"email", safeString(4)},
            {"birthday", safeString(5, false)},
            {"head_image", safeString(6)},
        };
        appendUserNameParts(user_data, userName);
        break;
    }
    return user_data;
}

crow::response userHandler::userUpdate(const crow::request &req, int userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        // 获取更新字段(整数类型)
        const auto getOptionalInt = [&request_body](const std::string &key, int defaultValue) -> int
        {
            auto item = request_body.find(key);
            return item == request_body.end() || item->is_null() ? defaultValue : item->get<int>();
        };

        // 获取更新字段(浮点类型)
        const auto getOptionalDouble = [&request_body](const std::string &key, double defaultValue) -> double
        {
            return getRequestDouble(request_body, key, defaultValue);
        };

        // 规范生日字段格式
        const auto normalizeBirthday = [](std::string rawBirthday) -> std::string
        {
            if (rawBirthday.empty())
            {
                return "";
            }

            try
            {
                boost::gregorian::date parsed_date;

                if (rawBirthday.length() >= 8)
                {
                    if (rawBirthday.length() == 10 && rawBirthday[4] == '-' && rawBirthday[7] == '-')
                    {
                        parsed_date = boost::gregorian::from_simple_string(rawBirthday);
                    }
                    else if (std::regex_match(rawBirthday, std::regex(R"(^\d{4}-\d{1,2}-\d{1,2}$)")))
                    {
                        parsed_date = boost::gregorian::from_simple_string(rawBirthday);
                    }
                    else if (std::regex_match(rawBirthday, std::regex(R"(^\d{4}-[A-Za-z]{3}-\d{1,2}$)")))
                    {
                        std::istringstream iss(rawBirthday);
                        iss.imbue(std::locale(std::locale::classic(),
                                              new boost::gregorian::date_input_facet("%Y-%b-%d")));
                        iss >> parsed_date;
                    }
                    else
                    {
                        parsed_date = boost::gregorian::from_simple_string(rawBirthday);
                    }

                    std::ostringstream oss;
                    oss << std::setfill('0') << std::setw(4) << parsed_date.year()
                        << "-" << std::setfill('0') << std::setw(2) << static_cast<unsigned>(parsed_date.month())
                        << "-" << std::setfill('0') << std::setw(2) << parsed_date.day();
                    return oss.str();
                }
            }
            catch (const std::exception &e)
            {
                std::cout << "Debug: Exception in birthday normalization: " << e.what() << ", using default" << std::endl;
                return "1970-01-01";
            }

            return rawBirthday;
        };

        // 注册时存储的数据
        if (userId <= 0)
        {
            int type_id = RoleTypeUtils::getRoleId(dbManager, "普通用户");
            std::string name = getRequestString(request_body, "name", "");
            std::string password = getRequestString(request_body, "password", "");
            std::string phone = getRequestString(request_body, "phone", "");
            std::string email = getRequestString(request_body, "email", "");
            std::string birthday = normalizeBirthday(getRequestString(request_body, "birthday", "1970-01-01"));
            std::string head_image = getRequestStringWithFallback(request_body, "head_image", "headImage", "");
            std::string user_specialty = getRequestString(request_body, "user_specialty", "");
            std::string user_introduction = getRequestString(request_body, "user_introduction", "");
            int user_level = getOptionalInt("user_level", 0);
            double funds = getOptionalDouble("funds", 0.0);
            int is_deleted = getOptionalInt("is_deleted", 0);
            int deleted_by = getOptionalInt("deleted_by", 0);
            std::string deleted_at = getRequestString(request_body, "deleted_at", "");

            if (password.empty())
            {
                return ResponseHelper::validation(req, "注册用户必须提供密码");
            }
            if (email.empty() && phone.empty())
            {
                return ResponseHelper::validation(req, "注册用户必须提供邮箱或手机号");
            }
            if (name.empty())
            {
                name = !email.empty() ? email : (!phone.empty() ? phone : "未命名");
            }
            if (birthday.empty())
            {
                birthday = "1970-01-01";
            }

            const std::string hashed_password = hash_password(password);

            auto session = dbManager->getSession();
            session->sql("START TRANSACTION").execute();

            try
            {
                mysqlx::SqlResult result = session->sql("INSERT INTO users(type_id, name, phone, email, password, birthday, head_image, user_specialty, user_introduction, user_level, funds, is_deleted, deleted_by, deleted_at) "
                                                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
                                               .bind(type_id, name, phone, email, hashed_password, birthday, head_image, user_specialty, user_introduction, user_level, funds, is_deleted, deleted_by, deleted_at)
                                               .execute();

                // 用户注册过程中，若手机号同步失败，则回滚整个事务，确保数据一致性
                if (!UserPhoneSync::upsertUserPhone(*session, static_cast<int>(result.getAutoIncrementValue()), phone))
                {
                    session->sql("ROLLBACK").execute();
                    return ResponseHelper::error(req, "用户注册失败，手机号同步未完成");
                }

                session->sql("COMMIT").execute();
                AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
            }
            catch (...)
            {
                session->sql("ROLLBACK").execute();
                throw;
            }

            return ResponseHelper::success(req, "用户注册成功");
        }

        // 用户资料更新只处理普通资料，邮箱、手机号、密码交给 security 下的独立接口。
        bool hasUpdateField = request_body.contains("name") ||
                              request_body.contains("birthday") ||
                              request_body.contains("head_image") ||
                              request_body.contains("headImage");

        if (!hasUpdateField)
        {
            return ResponseHelper::error(req, "请求中没有可更新的用户资料字段");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT name, CAST(birthday AS CHAR), head_image "
                                             "FROM users WHERE id = ? "
                                             "LIMIT 1")
                                       .bind(userId)
                                       .execute();

        auto row = result.fetchOne();
        if (!row)
        {
            return ResponseHelper::notFound(req, "User not found");
        }

        const std::string DBname = row[0].isNull() ? "" : row[0].get<std::string>();
        const std::string DBbirthday = row[1].isNull() ? "1970-01-01" : row[1].get<std::string>();
        const std::string DBhead_image = row[2].isNull() ? "" : row[2].get<std::string>();

        std::string name = getRequestStringWithFallback(request_body, "name", "name", DBname);
        std::string birthday = normalizeBirthday(getRequestStringWithFallback(request_body, "birthday", "birthady", DBbirthday));
        std::string head_image = getRequestStringWithFallback(request_body, "head_image", "headImage", DBhead_image);
        if (birthday.empty())
        {
            birthday = "1970-01-01";
        }

        bool has_changes = (!name.empty() && DBname != name) ||
                           (!birthday.empty() && birthday != DBbirthday) ||
                           (!head_image.empty() && DBhead_image != head_image);

        if (!has_changes)
        {
            return ResponseHelper::success(req, "No changes to update");
        }

        if (!head_image.empty() && DBhead_image != head_image)
        {
            // 删除原来的图片，如果文件不存在也不会报错
            const std::string lastFileName = getLastFileName(DBhead_image);
            std::string oldFilePath = std::string(UPLOADS_DIR) + "/" + lastFileName;

            // 检查文件是否存在后再删除，避免删除目录
            if (std::filesystem::exists(oldFilePath) && !std::filesystem::is_directory(oldFilePath))
            {
                try
                {
                    std::filesystem::remove(oldFilePath);
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error removing old avatar file: " << e.what() << std::endl;
                    OperationLogger::LogExceptionOperation(dbManager, req, "用户", "更新头像", "Failed to remove old avatar file: " + std::string(e.what()));
                }
            }
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            session->sql("UPDATE users SET name = ?, birthday = ?, head_image = ? "
                         "WHERE id = ? AND is_deleted = 0")
                .bind(name, birthday, head_image, userId)
                .execute();
            session->sql("COMMIT").execute();
        }
        catch (const mysqlx::Error &e)
        {
            session->sql("ROLLBACK").execute();
            return ResponseHelper::database_error(req, "Failed to update user data", e.what());
        }

        nlohmann::json response;
        response["user"] = getUserData(userId);

        return ResponseHelper::success(req, response);
    }
    catch (const mysqlx::Error &e)
    {
        return ResponseHelper::database_error(req, "Database connection failed", e.what());
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "用户", "更新用户数据", "Exception occurred: " + std::string(e.what()));
        return ResponseHelper::operation_failed(req, "Failed to update user data", e.what());
    }
}

crow::response userHandler::updatePassword(const crow::request &req, int userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string password = getRequestStringWithFallback(request_body, "password", "newPassword", "");
        if (userId <= 0)
        {
            return ResponseHelper::error(req, "无效的用户ID");
        }
        if (password.empty())
        {
            return ResponseHelper::validation(req, "新密码不能为空");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT password FROM users "
                                             "WHERE id = ? AND is_deleted = 0 "
                                             "LIMIT 1")
                                       .bind(userId)
                                       .execute();
        auto row = result.fetchOne();
        if (!row)
        {
            return ResponseHelper::notFound(req, "User not found");
        }

        const std::string DBpassword = row[0].isNull() ? "" : row[0].get<std::string>();
        bool password_matches = false; // 密码匹配标志
        try
        {
            password_matches = verify_password_hash(password, DBpassword);
        }
        catch (...)
        {
            password_matches = false;
        }

        bool password_needs_upgrade = false; // 密码需要升级标志
        if (password_matches)
        {
            try
            {
                password_needs_upgrade = password_hash_needs_upgrade(DBpassword);
            }
            catch (...)
            {
                password_needs_upgrade = true;
            }
        }

        if (password_matches && !password_needs_upgrade)
        {
            return ResponseHelper::success(req, "No changes to update");
        }

        dbManager->getSession()
            ->sql("UPDATE users SET password = ? WHERE id = ?")
            .bind(hash_password(password), userId)
            .execute();

        return ResponseHelper::success(req, "密码更新成功");
    }
    catch (const mysqlx::Error &e)
    {
        return ResponseHelper::database_error(req, "Failed to update password", e.what());
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "用户", "更新密码", "Exception occurred: " + std::string(e.what()));
        return ResponseHelper::operation_failed(req, "Failed to update password", e.what());
    }
}

crow::response userHandler::updateEmail(const crow::request &req, int userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string email = getRequestString(request_body, "email", "");
        std::string ticket = getRequestString(request_body, "ticket", "");
        if (userId <= 0)
        {
            return ResponseHelper::error(req, "无效的用户ID");
        }
        if (email.empty() || !isValidEmailFormat(email))
        {
            return ResponseHelper::validation(req, "邮箱不能为空或邮箱格式不正确");
        }
        if (ticket.empty())
        {
            return ResponseHelper::validation(req, "邮箱验证凭证输入错误");
        }

        auto ticketClaims = JwtUtils::getUpdateTicketClaims(ticket, email, "email");
        if (!ticketClaims || ticketClaims->userId != userId || ticketClaims->data != email || ticketClaims->identifier != "email")
        {
            return ResponseHelper::permission_denied(req, "邮箱验证凭证无效或已过期");
        }

        mysqlx::SqlResult result1 = dbManager->getSession()
                                        ->sql("SELECT id "
                                              "FROM users "
                                              "WHERE email = ? AND id <> ? AND is_deleted = 0 "
                                              "LIMIT 1")
                                        .bind(email, userId)
                                        .execute();

        if (result1.fetchOne())
        {
            return ResponseHelper::validation(req, "邮箱已被其他用户使用");
        }

        mysqlx::SqlResult result2 = dbManager->getSession()
                                        ->sql("SELECT u.email, u.type_id, t.type "
                                              "FROM users AS u "
                                              "JOIN types AS t ON t.id = u.type_id "
                                              "WHERE u.id = ? AND u.is_deleted = 0 "
                                              "LIMIT 1")
                                        .bind(userId)
                                        .execute();
        auto row = result2.fetchOne();
        if (!row)
        {
            return ResponseHelper::notFound(req, "User not found");
        }

        const std::string DBemail = row[0].isNull() ? "" : row[0].get<std::string>();
        const int type_id = row[1].isNull() ? 0 : row[1].get<int>();
        const std::string type = row[2].isNull() ? "" : row[2].get<std::string>();
        if (constantTimeEquals(email, DBemail))
        {
            return ResponseHelper::success(req, "No changes to update");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            mysqlx::SqlResult result3 = session->sql("UPDATE users SET email = ? "
                                                     "WHERE id = ? AND is_deleted = 0")
                                            .bind(email, userId)
                                            .execute();

            if (result3.getAffectedItemsCount() == 0)
            {
                session->sql("ROLLBACK").execute();
                return ResponseHelper::database_error(req, "更新邮箱失败");
            }

            session->sql("COMMIT").execute();
        }
        catch (...)
        {
            session->sql("ROLLBACK").execute();
            throw;
        }

        nlohmann::json response;
        response["message"] = "邮箱更新成功";
        response["user"] = getUserData(userId);
        response["token"] = JwtUtils::createToken(
            userId,
            type_id,
            type,
            email,
            true);
        return ResponseHelper::success(req, response);
    }
    catch (const mysqlx::Error &e)
    {
        return ResponseHelper::database_error(req, "Failed to update email", e.what());
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "用户", "更新邮箱", "Exception occurred: " + std::string(e.what()));
        return ResponseHelper::operation_failed(req, "Failed to update email", e.what());
    }
}

crow::response userHandler::updatePhone(const crow::request &req, int userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string phone = normalizePhoneIdentifier(getRequestString(request_body, "phone", ""));
        std::string ticket = getRequestString(request_body, "ticket", "");
        if (userId <= 0)
        {
            return ResponseHelper::error(req, "无效的用户ID");
        }
        if (phone.empty())
        {
            return ResponseHelper::validation(req, "手机号不能为空");
        }
        if (ticket.empty())
        {
            return ResponseHelper::validation(req, "手机验证凭证输入错误");
        }

        auto ticketClaims = JwtUtils::getUpdateTicketClaims(ticket, phone, "phone");
        if (!ticketClaims || ticketClaims->userId != userId || ticketClaims->data != phone || ticketClaims->identifier != "phone")
        {
            return ResponseHelper::permission_denied(req, "手机验证凭证无效或已过期");
        }

        mysqlx::SqlResult result1 = dbManager->getSession()
                                        ->sql("SELECT id "
                                              "FROM users "
                                              "WHERE phone = ? AND id <> ? AND is_deleted = 0 "
                                              "LIMIT 1")
                                        .bind(phone, userId)
                                        .execute();

        if (result1.fetchOne())
        {
            return ResponseHelper::validation(req, "手机号已被其他用户使用");
        }

        mysqlx::SqlResult result2 = dbManager->getSession()
                                        ->sql("SELECT u.phone, u.type_id, t.type "
                                              "FROM users AS u "
                                              "JOIN types AS t ON t.id = u.type_id "
                                              "WHERE u.id = ? AND u.is_deleted = 0 "
                                              "LIMIT 1")
                                        .bind(userId)
                                        .execute();

        auto row = result2.fetchOne();
        if (!row)
        {
            return ResponseHelper::notFound(req, "User not found");
        }

        const std::string DBphone = row[0].isNull() ? "" : normalizePhoneIdentifier(row[0].get<std::string>());
        const int type_id = row[1].isNull() ? 0 : row[1].get<int>();
        const std::string type = row[2].isNull() ? "" : row[2].get<std::string>();

        if (constantTimeEquals(DBphone, phone))
        {
            return ResponseHelper::success(req, "No changes to update");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            mysqlx::SqlResult result3 = session->sql("UPDATE users SET phone = ? "
                                                     "WHERE id = ? AND is_deleted = 0")
                                            .bind(phone, userId)
                                            .execute();

            if (result3.getAffectedItemsCount() == 0)
            {
                return ResponseHelper::database_error(req, "更新手机号失败");
            }

            if (!UserPhoneSync::upsertUserPhone(*session, userId, phone)) // 同步手机号
            {
                session->sql("ROLLBACK").execute();
                return ResponseHelper::system_error(req, "手机号更新失败，手机号同步未完成");
            }

            session->sql("COMMIT").execute();
        }
        catch (...)
        {
            session->sql("ROLLBACK").execute();
            throw;
        }

        nlohmann::json response;
        response["message"] = "手机号更新成功";
        response["user"] = getUserData(userId);
        response["token"] = JwtUtils::createToken(
            userId,
            type_id,
            type,
            phone,
            false);
        return ResponseHelper::success(req, response);
    }
    catch (const mysqlx::Error &e)
    {
        return ResponseHelper::database_error(req, "Failed to update phone", e.what());
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "用户", "更新手机号", "Exception occurred: " + std::string(e.what()));
        return ResponseHelper::operation_failed(req, "Failed to update phone", e.what());
    }
}

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

        const auto stripChinaCountryCode = [](const std::string &value)
        {
            return value.rfind("+86", 0) == 0 ? value.substr(3) : value;
        };

        // 从数据库中获取用户信息
        // user是一个智能指针
        std::unique_ptr<User> user = nullptr;
        try
        {
            // 获取表
            mysqlx::SqlResult result;
            if (!email.empty())
            {
                // 通过email查询用户
                result = dbManager->getSession()
                             ->sql("SELECT u.id, u.type_id, t.type, u.name, u.password, u.phone, u.email, "
                                   "CAST(u.birthday AS CHAR), u.head_image "
                                   "FROM users AS u "
                                   "LEFT JOIN types AS t ON u.type_id = t.id "
                                   "WHERE u.email = ?")
                             .bind(email)
                             .execute();
            }
            else if (!phone.empty())
            {
                const std::string legacyPhone = stripChinaCountryCode(phone);
                // 通过phone查询用户
                result = dbManager->getSession()
                             ->sql("SELECT u.id, u.type_id, t.type, u.name, u.password, u.phone, u.email, "
                                   "CAST(u.birthday AS CHAR), u.head_image "
                                   "FROM users AS u "
                                   "LEFT JOIN types AS t ON u.type_id = t.id "
                                   "WHERE u.phone = ? OR u.phone = ?")
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
            // 不区分用户不存在和密码错误，统一返回相同错误信息
            return ResponseHelper::error(req, "Invalid username or password");
        }
        else
        {
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
            const std::string role_name =
                RoleTypeUtils::getRoleName(dbManager, user->getTypeID());
            std::string token = JwtUtils::createToken(
                user->getID(),
                user->getTypeID(),
                role_name,
                loginIdentifier,
                loggedInWithEmail);
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

crow::response userHandler::addNewAddress(const crow::request &req, int userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        if (userId <= 0)
        {
            return ResponseHelper::unauthorized(req, "Invalid user identity");
        }

        std::string contact_name = getRequestString(request_body, "contact_name");
        std::string contact_phone = getRequestString(request_body, "contact_phone");
        std::string country = "中国"; // 默认国家为中国
        std::string province = getRequestString(request_body, "province");
        std::string city = getRequestString(request_body, "city");
        std::string district = getRequestString(request_body, "district");
        std::string detail_address = getRequestString(request_body, "detail_address");
        std::string address_text = province + city + district + detail_address; // 用于地理编码的地址文本
        std::string postal_code = getRequestString(request_body, "postal_code");
        std::string address_tag = getRequestString(request_body, "address_tag", "家");
        std::string remarks = getRequestString(request_body, "remarks");
        double longitude = 0.0;
        double latitude = 0.0;
        std::string geocode_source = "";

        if (contact_name.empty() || contact_phone.empty() || province.empty() ||
            city.empty() || district.empty() || detail_address.empty())
        {
            return ResponseHelper::validation(req, "联系人、手机号、省、市、区和详细地址不能为空");
        }

        if (!address_text.empty())
        {
            // 调用地理编码函数获取经纬度和地理编码来源
            geocode(address_text, longitude, latitude, geocode_source);
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        try
        {
            mysqlx::SqlResult existingAddressResult = session->sql("SELECT id FROM address WHERE user_id = ? AND is_deleted = 0 LIMIT 1")
                                                          .bind(userId)
                                                          .execute();
            const int is_default = existingAddressResult.fetchOne() ? 0 : 1;

            mysqlx::SqlResult insert_result = session->sql("INSERT INTO address (user_id, contact_name, contact_phone, country, province, city, district, detail_address, address_text, postal_code, address_tag, is_default, longitude, latitude, geocode_source, remarks) "
                                                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
                                                  .bind(userId, contact_name, contact_phone, country, province, city, district, detail_address, address_text, postal_code, address_tag, is_default, longitude, latitude, geocode_source, remarks)
                                                  .execute();

            if (insert_result.getAffectedItemsCount() == 0)
            {
                session->sql("ROLLBACK").execute();
                return ResponseHelper::operation_failed(req, "Failed to insert new address");
            }

            const int address_id = static_cast<int>(insert_result.getAutoIncrementValue());

            session->sql("COMMIT").execute();

            mysqlx::SqlResult getAddress_result = session->sql(
                                                             "SELECT id, user_id, contact_name, contact_phone, country, province, city, district, "
                                                             "detail_address, address_text, postal_code, address_tag, is_default, longitude, latitude, remarks "
                                                             "FROM address "
                                                             "WHERE id = ? AND user_id = ? AND is_deleted = 0")
                                                      .bind(address_id, userId)
                                                      .execute();

            auto row = getAddress_result.fetchOne();
            if (!row)
            {
                return ResponseHelper::operation_failed(req, "New address was inserted but could not be loaded");
            }

            nlohmann::json data = {
                {"id", row[0].isNull() ? 0 : row[0].get<int>()},
                {"user_id", row[1].isNull() ? 0 : row[1].get<int>()},
                {"contact_name", row[2].isNull() ? "" : row[2].get<std::string>()},
                {"contact_phone", row[3].isNull() ? "" : row[3].get<std::string>()},
                {"country", row[4].isNull() ? "" : row[4].get<std::string>()},
                {"province", row[5].isNull() ? "" : row[5].get<std::string>()},
                {"city", row[6].isNull() ? "" : row[6].get<std::string>()},
                {"district", row[7].isNull() ? "" : row[7].get<std::string>()},
                {"detail_address", row[8].isNull() ? "" : row[8].get<std::string>()},
                {"address_text", row[9].isNull() ? "" : row[9].get<std::string>()},
                {"postal_code", row[10].isNull() ? "" : row[10].get<std::string>()},
                {"address_tag", row[11].isNull() ? "" : row[11].get<std::string>()},
                {"is_default", row[12].isNull() ? 0 : row[12].get<int>()},
                {"longitude", row[13].isNull() ? 0.0 : row[13].get<double>()},
                {"latitude", row[14].isNull() ? 0.0 : row[14].get<double>()},
                {"remarks", row[15].isNull() ? "" : row[15].get<std::string>()}};

            return ResponseHelper::success(req, data);
        }
        catch (...)
        {
            session->sql("ROLLBACK").execute();
            throw;
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "地址", "添加新地址", "Failed to add new address for user ID " + std::to_string(userId) + ": " + std::string(e.what()));
        return ResponseHelper::operation_failed(req, "Failed to add new address", e.what());
    }
}

crow::response userHandler::addressUpdate(const crow::request &req, int userId, int addressId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        if (userId <= 0)
        {
            return ResponseHelper::unauthorized(req, "Invalid user identity");
        }
        if (addressId <= 0)
        {
            return ResponseHelper::validation(req, "Invalid address ID");
        }

        if (request_body.contains("contact_name") || request_body.contains("contact_phone") ||
            request_body.contains("province") || request_body.contains("city") ||
            request_body.contains("district") || request_body.contains("detail_address"))
        {
            return ResponseHelper::validation(req, "联系人、手机号、省、市、区和详细地址不能为空");
        }

        auto session = dbManager->getSession();

        mysqlx::SqlResult result = session->sql(
                                              "SELECT id, user_id, contact_name, contact_phone, country, province, city, district, "
                                              "detail_address, address_text, postal_code, address_tag, is_default, longitude, latitude, "
                                              "geocode_source, remarks "
                                              "FROM address WHERE id = ? AND user_id = ? AND is_deleted = 0")
                                       .bind(addressId, userId)
                                       .execute();

        auto row = result.fetchOne();
        if (!row)
        {
            return ResponseHelper::notFound(req, "Address not found");
        }

        std::string DBcontact_name = row[2].isNull() ? "" : row[2].get<std::string>();
        std::string DBcontact_phone = row[3].isNull() ? "" : row[3].get<std::string>();
        std::string DBcountry = row[4].isNull() ? "中国" : row[4].get<std::string>();
        std::string DBprovince = row[5].isNull() ? "" : row[5].get<std::string>();
        std::string DBcity = row[6].isNull() ? "" : row[6].get<std::string>();
        std::string DBdistrict = row[7].isNull() ? "" : row[7].get<std::string>();
        std::string DBdetail_address = row[8].isNull() ? "" : row[8].get<std::string>();
        std::string DBaddress_text = row[9].isNull() ? "" : row[9].get<std::string>();
        std::string DBpostal_code = row[10].isNull() ? "" : row[10].get<std::string>();
        std::string DBaddress_tag = row[11].isNull() ? "" : row[11].get<std::string>();
        int DBis_default = row[12].isNull() ? 0 : row[12].get<int>();
        double DBlongitude = row[13].isNull() ? 0.0 : row[13].get<double>();
        double DBlatitude = row[14].isNull() ? 0.0 : row[14].get<double>();
        std::string DBgeocode_source = row[15].isNull() ? "" : row[15].get<std::string>();
        std::string DBremarks = row[16].isNull() ? "" : row[16].get<std::string>();

        std::string contact_name = getRequestStringWithFallback(request_body, "contact_name", "contact_name", DBcontact_name);
        std::string contact_phone = getRequestStringWithFallback(request_body, "contact_phone", "contactPhone", DBcontact_phone);
        std::string country = getRequestStringWithFallback(request_body, "country", "country", DBcountry);
        std::string province = getRequestStringWithFallback(request_body, "province", "province", DBprovince);
        std::string city = getRequestStringWithFallback(request_body, "city", "city", DBcity);
        std::string district = getRequestStringWithFallback(request_body, "district", "district", DBdistrict);
        std::string detail_address = getRequestStringWithFallback(request_body, "detail_address", "detailAddress", DBdetail_address);
        std::string postal_code = getRequestStringWithFallback(request_body, "postal_code", "postalCode", DBpostal_code);
        std::string address_tag = getRequestStringWithFallback(request_body, "address_tag", "addressTag", DBaddress_tag);
        std::string remarks = getRequestStringWithFallback(request_body, "remarks", "remarks", DBremarks);

        if (address_tag != "家" && address_tag != "公司" && address_tag != "医院" &&
            address_tag != "学校" && address_tag != "其他")
        {
            return ResponseHelper::validation(req, "地址标签无效");
        }

        const bool addressBodyChanged = province != DBprovince ||
                                        city != DBcity ||
                                        district != DBdistrict ||
                                        detail_address != DBdetail_address;
        std::string address_text = province + city + district + detail_address;
        double longitude = DBlongitude;
        double latitude = DBlatitude;
        std::string geocode_source = DBgeocode_source;

        // 当地址主体信息发生变化时，才重新进行地理编码获取经纬度和地理编码来源
        if (addressBodyChanged)
        {
            longitude = 0.0;
            latitude = 0.0;
            geocode_source.clear();
            geocode(address_text, longitude, latitude, geocode_source);
        }

        const bool has_changes = contact_name != DBcontact_name ||
                                 contact_phone != DBcontact_phone ||
                                 country != DBcountry ||
                                 address_text != DBaddress_text ||
                                 postal_code != DBpostal_code ||
                                 address_tag != DBaddress_tag ||
                                 remarks != DBremarks;

        if (!has_changes)
        {
            return ResponseHelper::success(req, "Address has no changes");
        }

        session->sql("START TRANSACTION").execute();
        try
        {
            mysqlx::SqlResult updateResult = session->sql(
                                                        "UPDATE address SET contact_name = ?, contact_phone = ?, country = ?, "
                                                        "province = ?, city = ?, district = ?, detail_address = ?, address_text = ?, "
                                                        "postal_code = ?, address_tag = ?, longitude = ?, latitude = ?, "
                                                        "geocode_source = ?, remarks = ? "
                                                        "WHERE id = ? AND user_id = ? AND is_deleted = 0")
                                                 .bind(contact_name, contact_phone, country, province, city, district,
                                                       detail_address, address_text, postal_code, address_tag,
                                                       longitude, latitude, geocode_source, remarks, addressId, userId)
                                                 .execute();

            if (updateResult.getAffectedItemsCount() == 0)
            {
                session->sql("ROLLBACK").execute();
                return ResponseHelper::notFound(req, "Address not found");
            }

            mysqlx::SqlResult updatedResult = session->sql(
                                                         "SELECT id, user_id, contact_name, contact_phone, country, province, city, district, "
                                                         "detail_address, address_text, postal_code, address_tag, is_default, longitude, latitude, remarks "
                                                         "FROM address WHERE id = ? AND user_id = ? AND is_deleted = 0")
                                                  .bind(addressId, userId)
                                                  .execute();
            auto updatedRow = updatedResult.fetchOne();
            if (!updatedRow)
            {
                session->sql("ROLLBACK").execute();
                return ResponseHelper::notFound(req, "Address not found after update");
            }

            nlohmann::json data = {
                {"id", updatedRow[0].isNull() ? 0 : updatedRow[0].get<int>()},
                {"user_id", updatedRow[1].isNull() ? 0 : updatedRow[1].get<int>()},
                {"contact_name", updatedRow[2].isNull() ? "" : updatedRow[2].get<std::string>()},
                {"contact_phone", updatedRow[3].isNull() ? "" : updatedRow[3].get<std::string>()},
                {"country", updatedRow[4].isNull() ? "" : updatedRow[4].get<std::string>()},
                {"province", updatedRow[5].isNull() ? "" : updatedRow[5].get<std::string>()},
                {"city", updatedRow[6].isNull() ? "" : updatedRow[6].get<std::string>()},
                {"district", updatedRow[7].isNull() ? "" : updatedRow[7].get<std::string>()},
                {"detail_address", updatedRow[8].isNull() ? "" : updatedRow[8].get<std::string>()},
                {"address_text", updatedRow[9].isNull() ? "" : updatedRow[9].get<std::string>()},
                {"postal_code", updatedRow[10].isNull() ? "" : updatedRow[10].get<std::string>()},
                {"address_tag", updatedRow[11].isNull() ? "" : updatedRow[11].get<std::string>()},
                {"is_default", updatedRow[12].isNull() ? 0 : updatedRow[12].get<int>()},
                {"longitude", updatedRow[13].isNull() ? 0.0 : updatedRow[13].get<double>()},
                {"latitude", updatedRow[14].isNull() ? 0.0 : updatedRow[14].get<double>()},
                {"remarks", updatedRow[15].isNull() ? "" : updatedRow[15].get<std::string>()}};

            session->sql("COMMIT").execute();
            return ResponseHelper::success(req, data);
        }
        catch (...)
        {
            session->sql("ROLLBACK").execute();
            throw;
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "地址", "更新地址", "Failed to update address with ID " + std::to_string(addressId) + ": " + std::string(e.what()));
        return ResponseHelper::operation_failed(req, "Failed to update address", e.what());
    }
}

crow::response userHandler::userUploadAvatar(const crow::request &req)
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
                    return ResponseHelper::validation(req, "Invalid file type. Only image files are allowed.");
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
                    return ResponseHelper::operation_failed(req, "Failed to save file", "Unable to write avatar file to disk");
                }
            }
        }

        // 构建响应
        std::string avatar_url = "http://localhost:8081/uploads/" + unique_filename;
        nlohmann::json response;
        response["message"] = "File uploaded successfully";
        response["avatarUrl"] = avatar_url;
        response["filename"] = unique_filename;

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "用户", "上传头像", "Exception occurred during avatar upload: " + std::string(e.what()));
        return ResponseHelper::system_error(req);
    }
}

crow::response userHandler::upload(const crow::request &req, const std::string &filename)
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

namespace
{
    std::string getTodayDate()
    {
        const boost::posix_time::ptime currentDateTime = boost::posix_time::second_clock::local_time();
        return formatDateOnly(currentDateTime);
    }

    nlohmann::json buildDoctorJson(const mysqlx::Row &row)
    {
        nlohmann::json doctor;
        doctor["doctor_id"] = row[0].isNull() ? 0 : row[0].get<int>();
        doctor["id"] = doctor["doctor_id"];
        doctor["name"] = row[1].isNull() ? "" : row[1].get<std::string>();
        doctor["phone"] = row[2].isNull() ? "" : row[2].get<std::string>();
        doctor["email"] = row[3].isNull() ? "" : row[3].get<std::string>();
        doctor["specialty"] = row[4].isNull() ? "" : row[4].get<std::string>();
        doctor["status"] = row[5].isNull() ? "offline" : row[5].get<std::string>();
        return doctor;
    }

    bool isValidReservationStatus(const std::string &status)
    {
        return status == "预约成功" || status == "预约失败" || status == "已取消" || status == "已到院";
    }
}

// 创建预约表记录接口
crow::response userHandler::createReservation(const crow::request &req, int user_id, int pet_id, int doctor_id, std::string reservation_type, std::string date, std::string time_slot, std::string status)
{
    try
    {
        // 检查数据库连接是否存在
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "创建预约", "database connection failed", user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        // 检查必要字段是否存在
        if (!isValidReservationStatus(status))
        {
            return ResponseHelper::validation(req, "预约状态不合法");
        }

        if (reservation_type.size() > 30)
        {
            return ResponseHelper::validation(req, "预约类型不能超过30个字符");
        }

        if (user_id > 0 && pet_id > 0 && doctor_id > 0 && !reservation_type.empty() && !date.empty() && !time_slot.empty())
        {
            try
            {
                mysqlx::SqlResult petResult = dbManager->getSession()
                                                  ->sql("SELECT COUNT(*) FROM pets WHERE id = ? AND user_id = ? AND is_deleted = 0")
                                                  .bind(pet_id, user_id)
                                                  .execute();
                auto petRow = petResult.fetchOne();
                if (!petRow || petRow[0].get<int>() == 0)
                {
                    return ResponseHelper::validation(req, "宠物不存在或不属于当前用户");
                }

                mysqlx::SqlResult slotResult = dbManager->getSession()
                                                   ->sql("SELECT id FROM reaservations "
                                                         "WHERE doctor_id = ? AND date = ? AND time_slot = ? "
                                                         "AND COALESCE(status, '预约成功') NOT IN ('已取消', '预约失败') "
                                                         "LIMIT 1")
                                                   .bind(doctor_id, date, time_slot)
                                                   .execute();
                if (slotResult.fetchOne())
                {
                    return ResponseHelper::validation(req, "该医生当前时间段已被预约");
                }

                mysqlx::SqlResult insertResult = dbManager->getSession()
                                                     ->sql("INSERT INTO reaservations (user_id, pet_id, doctor_id, reservation_type, date, time_slot, status) "
                                                           "VALUES (?, ?, ?, ?, ?, ?, ?)")
                                                     .bind(user_id, pet_id, doctor_id, reservation_type, date, time_slot, status)
                                                     .execute();

                uint64_t reservationId = insertResult.getAutoIncrementValue();

                mysqlx::SqlResult createdResult = dbManager->getSession()
                                                      ->sql("SELECT r.id, r.user_id, r.pet_id, r.doctor_id, COALESCE(p.pet_name, ''), "
                                                            "COALESCE(r.reservation_type, ''), CAST(r.date AS CHAR), COALESCE(r.time_slot, ''), COALESCE(r.status, ''), "
                                                            "CAST(r.created_at AS CHAR) "
                                                            "FROM reaservations AS r "
                                                            "LEFT JOIN pets AS p ON r.pet_id = p.id "
                                                            "WHERE r.id = ? LIMIT 1")
                                                      .bind(reservationId)
                                                      .execute();
                auto createdRow = createdResult.fetchOne();

                nlohmann::json response;
                response["reservation_status"] = status;
                response["message"] = "预约成功";

                if (createdRow)
                {
                    const std::string createdReservationType = createdRow[5].isNull() ? "" : createdRow[5].get<std::string>();
                    const std::string createdDate = createdRow[6].isNull() ? "" : createdRow[6].get<std::string>();
                    const std::string createdSlot = createdRow[7].isNull() ? "" : createdRow[7].get<std::string>();
                    response["id"] = createdRow[0].get<int>();
                    response["user_id"] = createdRow[1].get<int>();
                    response["pet_id"] = createdRow[2].get<int>();
                    response["doctor_id"] = createdRow[3].get<int>();
                    response["pet_name"] = createdRow[4].isNull() ? "" : createdRow[4].get<std::string>();
                    response["reservation_type"] = createdReservationType;
                    response["reservationType"] = createdReservationType;
                    response["date"] = createdDate;
                    response["time_slot"] = createdSlot;
                    response["status"] = createdRow[8].isNull() ? status : createdRow[8].get<std::string>();
                    response["created_at"] = createdRow[9].isNull() ? "" : createdRow[9].get<std::string>();
                    response["price"] = 0;
                }

                return ResponseHelper::success(req, response);
            }
            catch (const mysqlx::Error &e)
            {
                std::cerr << "Database error: " << e.what() << std::endl;
                OperationLogger::LogExceptionOperation(dbManager, req, "预约", "创建预约", e.what(), user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
                return ResponseHelper::database_error(req, "Failed to create reservation", e.what());
            }
        }
        else
        {
            return ResponseHelper::validation(req, "Missing required fields");
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "创建预约", e.what(), user_id > 0 ? std::optional<int>(user_id) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to save reservation", e.what());
    }
}

// 获取预约时间表数据接口
nlohmann::json userHandler::getReservationDate()
{
    // 这里应该生成并返回时间表
    Reservate r;
    auto schedule = r.generateSchedule();
    return schedule;
}

// 获取医生列表接口
crow::response userHandler::getDoctorList(const crow::request &req)
{
    if (!checkDbConnection())
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取医生列表", "database connection failed");
        return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
    }

    try
    {
        const int doctorRoleId = RoleTypeUtils::getRoleId(dbManager, "医生");
        if (doctorRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "医生角色不存在");
        }

        const std::string todayDate = getTodayDate();

        mysqlx::RowResult result = dbManager->getSession()
                                       ->sql("SELECT u.id, u.name, u.phone, u.email, u.user_specialty, "
                                             "COALESCE(od.status, 'offline') "
                                             "FROM users AS u "
                                             "LEFT JOIN onlineDoctors AS od "
                                             "ON od.doctor_id = u.id AND od.date = ? "
                                             "WHERE u.type_id = ? AND u.is_deleted = 0")
                                       .bind(todayDate, doctorRoleId)
                                       .execute();

        nlohmann::json doctorList = nlohmann::json::array();
        for (const auto &row : result)
        {
            doctorList.push_back(buildDoctorJson(row));
        }

        return ResponseHelper::success(req, doctorList);
    }
    catch (const mysqlx::Error &e)
    {
        std::cerr << "Database error: " << e.what() << std::endl;
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取医生列表", e.what());
        return ResponseHelper::database_error(req, "Failed to fetch doctor list", e.what());
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "获取医生列表", e.what());
        return ResponseHelper::operation_failed(req, "Failed to fetch doctor list", e.what());
    }
}

// 取消预约接口
crow::response userHandler::cancelReservation(const crow::request &req, int userId, int reservationId)
{
    try
    {
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "取消预约", "database connection failed", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        if (userId <= 0 || reservationId <= 0)
        {
            return ResponseHelper::validation(req, "Invalid reservation id");
        }

        std::string status = "已取消";

        // 验证用户和预约记录是否匹配
        mysqlx::SqlResult reservation_result = dbManager->getSession()
                                                   ->sql("SELECT user_id FROM reaservations WHERE id = ? AND user_hidden = 0")
                                                   .bind(reservationId)
                                                   .execute();

        auto reservation_row = reservation_result.fetchOne();
        if (!reservation_row)
        {
            return ResponseHelper::notFound(req, "Reservation record does not exist");
        }

        if (reservation_row[0].get<int>() == userId) // 操作用户和预约记录用户匹配
        {

            mysqlx::SqlResult result = dbManager->getSession()
                                           ->sql("UPDATE reaservations SET status = ? WHERE id = ?")
                                           .bind(status, reservationId)
                                           .execute();

            // 检查是否有记录被更新
            if (result.getAffectedItemsCount() > 0)
            {
                // 返回成功响应
                nlohmann::json response;
                response["message"] = "取消成功";
                response["reservation_id"] = reservationId;
                response["status"] = status;
                return ResponseHelper::success(req, response);
            }
            else
            {
                return ResponseHelper::notFound(req, "未找到指定的预约记录");
            }
        }
        else // 操作用户和预约记录用户不匹配
        {
            return ResponseHelper::permission_denied(
                req,
                "预约记录不匹配",
                "Reservation record does not belong to current user");
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "取消预约", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to cancel reservation", e.what());
    }
}

// 删除预约记录接口
crow::response userHandler::deleteReservation(const crow::request &req, int userId, int reservationId)
{
    try
    {
        // 检查数据库连接是否存在
        if (!checkDbConnection())
        {
            OperationLogger::LogExceptionOperation(dbManager, req, "预约", "删除预约记录", "database connection failed", userId > 0 ? std::optional<int>(userId) : std::nullopt);
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        mysqlx::SqlResult reservation_result = dbManager->getSession()
                                                   ->sql("SELECT user_id FROM reaservations WHERE id = ?")
                                                   .bind(reservationId)
                                                   .execute();

        auto reservation_row = reservation_result.fetchOne();

        if (!reservation_row)
        {
            return ResponseHelper::notFound(req, "Reservation record does not exist");
        }

        if (reservation_row[0].get<int>() == userId) // 操作用户和预约记录用户匹配才允许删除
        {

            // 用户侧删除只隐藏预约记录，医生端和管理端仍可查看历史记录。
            mysqlx::SqlResult result = dbManager->getSession()
                                           ->sql("UPDATE reaservations "
                                                 "SET user_hidden = 1, user_hidden_at = NOW(), hidden_by = ? "
                                                 "WHERE id = ? AND user_id = ? AND user_hidden = 0")
                                           .bind(userId, reservationId, userId)
                                           .execute();

            // 检查是否有记录被删除
            if (result.getAffectedItemsCount() > 0)
            {
                // 返回成功响应
                nlohmann::json response;
                response["message"] = "预约记录删除成功";
                response["reservation_id"] = reservationId;
                return ResponseHelper::success(req, response);
            }
            else
            {
                return ResponseHelper::notFound(req, "未找到指定的预约记录");
            }
        }
        else
        {
            return ResponseHelper::permission_denied(
                req,
                "预约记录不匹配",
                "Reservation record does not belong to current user");
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "预约", "删除预约记录", e.what(), userId > 0 ? std::optional<int>(userId) : std::nullopt);
        return ResponseHelper::operation_failed(req, "Failed to delete reservation", e.what());
    }
}
