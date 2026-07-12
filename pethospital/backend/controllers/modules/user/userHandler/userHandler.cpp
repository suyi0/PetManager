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
#include "userHandlerInternal.h" // 共享的用户名拆分辅助（与 userHandlerAuth.cpp 共用）
#include <vector>

namespace
{
    // 上传文件目录：优先环境变量 UPLOADS_DIR，默认放到项目 data/uploads（不再写进前端源码树）。
    // 文件由后端 /uploads/<file> 端点读取回传，故无需位于前端资源目录。只解析一次并缓存。
    const std::string &uploadsDir()
    {
        static const std::string dir = getEnvVar("UPLOADS_DIR", getProjectRoot() + "/data/uploads");
        return dir;
    }

    // 对外可访问的基础 URL（拼上传文件绝对地址用）：环境变量 PUBLIC_BASE_URL，
    // 默认开发环境的 http://localhost:8081；部署时指向真实域名/反代地址。
    const std::string &publicBaseUrl()
    {
        static const std::string url = getEnvVar("PUBLIC_BASE_URL", "http://localhost:8081");
        return url;
    }

    // 业务身份判据是 staff_kind（不可变工种标记），不是可改名的职位显示名
    void notifyDoctorListChangedIfDoctor(const std::string &staffKind)
    {
        if (staffKind == "doctor")
        {
            DoctorListCache::invalidateDoctorList();
            DoctorListBroadcaster::instance().notifyDoctorListChanged();
        }
    }

    // 调用点手头只有 userId 时按库里的当前工种判断
    void notifyDoctorListChangedIfDoctorByUser(
        const std::shared_ptr<DatabaseManagerInterface> &dbManager, int userId)
    {
        if (!dbManager || !dbManager->getSession() || userId <= 0)
        {
            return;
        }
        try
        {
            mysqlx::SqlResult result = dbManager->getSession()
                                           ->sql("SELECT COALESCE(pos.staff_kind, '') FROM users AS u "
                                                 "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                                 "WHERE u.id = ? LIMIT 1")
                                           .bind(userId)
                                           .execute();
            auto row = result.fetchOne();
            if (row && !row[0].isNull())
            {
                notifyDoctorListChangedIfDoctor(row[0].get<std::string>());
            }
        }
        catch (...)
        {
            // 列表刷新是尽力而为的通知，不因它失败中断主流程
        }
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
                                   ->sql("SELECT u.id, u.name, u.password, p.phone, u.email, CAST(u.birthday AS CHAR), u.head_image "
                                         "FROM users AS u "
                                         "LEFT JOIN phones AS p ON p.user_id = u.id "
                                         "WHERE u.id = ?")
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
                mysqlx::SqlResult result = session->sql("INSERT INTO users(account_type, position_id, name, email, password, birthday, head_image, user_specialty, user_introduction, user_level, funds, is_deleted, deleted_by, deleted_at) "
                                                        "VALUES ('customer', NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
                                               .bind(name, email, hashed_password, birthday, head_image, user_specialty, user_introduction, user_level, funds, is_deleted, deleted_by, deleted_at)
                                               .execute();

                // 用户注册过程中，若手机号同步失败，则回滚整个事务，确保数据一致性
                if (!UserPhoneSync::upsertUserPhone(*session, static_cast<int>(result.getAutoIncrementValue()), phone))
                {
                    rollbackTransactionQuietly(*session);
                    return ResponseHelper::error(req, "用户注册失败，手机号同步未完成");
                }

                session->sql("COMMIT").execute();
                AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
            }
            catch (...)
            {
                rollbackTransactionQuietly(*session);
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
                                       ->sql("SELECT u.name, CAST(u.birthday AS CHAR), u.head_image, "
                                             "COALESCE(pos.staff_kind, '') AS staff_kind "
                                             "FROM users AS u "
                                             "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                             "WHERE u.id = ? "
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
        const std::string staffKind = row[3].isNull() ? "" : row[3].get<std::string>();

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
            std::string oldFilePath = uploadsDir() + "/" + lastFileName;

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
            notifyDoctorListChangedIfDoctor(staffKind);
        }
        catch (const mysqlx::Error &e)
        {
            rollbackTransactionQuietly(*session);
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

        const std::string currentPassword = getRequestString(request_body, "currentPassword", "");
        const std::string newPassword = getRequestString(request_body, "newPassword", "");
        if (userId <= 0)
        {
            return ResponseHelper::error(req, "无效的用户ID");
        }
        if (currentPassword.empty() || newPassword.empty())
        {
            return ResponseHelper::validation(req, "当前密码和新密码不能为空");
        }
        if (newPassword.size() < 8 || newPassword.size() > 64 || !isValidPasswordFormat(newPassword))
        {
            return ResponseHelper::validation(req, "新密码需为8至64位，并同时包含字母和数字");
        }
        if (currentPassword == newPassword)
        {
            return ResponseHelper::validation(req, "新密码不能与当前密码相同");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT password FROM users AS u "
                                             "WHERE u.id = ? AND u.is_deleted = 0 "
                                             "LIMIT 1")
                                       .bind(userId)
                                       .execute();
        auto row = result.fetchOne();
        if (!row)
        {
            return ResponseHelper::notFound(req, "User not found");
        }

        const std::string storedPassword = row[0].isNull() ? "" : row[0].get<std::string>();
        bool currentPasswordMatches = false;
        try
        {
            currentPasswordMatches = verify_password_hash(currentPassword, storedPassword);
        }
        catch (...)
        {
            currentPasswordMatches = false;
        }

        if (!currentPasswordMatches)
        {
            return ResponseHelper::validation(req, "当前密码不正确");
        }

        dbManager->getSession()
            ->sql("UPDATE users SET password = ? WHERE id = ?")
            .bind(hash_password(newPassword), userId)
            .execute();

        // 改密后 bump 会话版本：吊销该用户在其他设备/端已签发的旧 token（旧版本立即失效）。
        AuthSessionStore::bumpSessionVersionForUser(userId);

        // 当前设备无缝续签：用当前身份重签一枚携带最新会话版本的 token 一并返回，
        // 避免本机被这次 bump 连带踢下线（否则前端只能强制重新登录，体验割裂）。
        // getTokenClaims 只解签名/过期取身份，不校验会话版本，故此刻旧 token 仍可解析。
        nlohmann::json response;
        response["message"] = "密码更新成功";
        const std::string authHeader = req.get_header_value("Authorization");
        if (authHeader.rfind("Bearer ", 0) == 0)
        {
            if (auto claims = JwtUtils::getTokenClaims(authHeader.substr(7)))
            {
                response["token"] = JwtUtils::createToken(
                    userId,
                    claims->typeId,
                    claims->typeName,
                    claims->identifier,
                    claims->isEmailLogin,
                    RbacService::userHasManagementAccess(dbManager, userId));
            }
        }

        return ResponseHelper::success(req, response);
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
                                        ->sql("SELECT u.email, COALESCE(u.position_id, 0), "
                                              "CASE WHEN u.account_type = 'customer' THEN '普通用户' ELSE COALESCE(pos.name, '') END AS type_name "
                                              "FROM users AS u "
                                              "LEFT JOIN positions AS pos ON pos.id = u.position_id "
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
                rollbackTransactionQuietly(*session);
                return ResponseHelper::database_error(req, "更新邮箱失败");
            }

            session->sql("COMMIT").execute();
        }
        catch (...)
        {
            rollbackTransactionQuietly(*session);
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
            true,
            RbacService::userHasManagementAccess(dbManager, userId));
        notifyDoctorListChangedIfDoctorByUser(dbManager, userId);
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
                                        ->sql("SELECT u.id "
                                              "FROM users AS u "
                                              "JOIN phones AS p ON p.user_id = u.id "
                                              "WHERE p.phone = ? AND u.id <> ? AND u.is_deleted = 0 "
                                              "LIMIT 1")
                                        .bind(phone, userId)
                                        .execute();

        if (result1.fetchOne())
        {
            return ResponseHelper::validation(req, "手机号已被其他用户使用");
        }

        mysqlx::SqlResult result2 = dbManager->getSession()
                                        ->sql("SELECT p.phone, COALESCE(u.position_id, 0), "
                                              "CASE WHEN u.account_type = 'customer' THEN '普通用户' ELSE COALESCE(pos.name, '') END AS type_name "
                                              "FROM users AS u "
                                              "LEFT JOIN positions AS pos ON pos.id = u.position_id "
                                              "LEFT JOIN phones AS p ON p.user_id = u.id "
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
            if (!UserPhoneSync::upsertUserPhone(*session, userId, phone)) // 同步手机号
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::system_error(req, "手机号更新失败，手机号同步未完成");
            }

            session->sql("COMMIT").execute();
        }
        catch (...)
        {
            rollbackTransactionQuietly(*session);
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
            false,
            RbacService::userHasManagementAccess(dbManager, userId));
        notifyDoctorListChangedIfDoctorByUser(dbManager, userId);
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
                if (!std::filesystem::exists(uploadsDir()))
                {
                    // 创建目录
                    std::filesystem::create_directories(uploadsDir());
                }

                filepath = uploadsDir() + "/" + unique_filename;

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
        std::string avatar_url = publicBaseUrl() + "/uploads/" + unique_filename;
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

    // 上传文件目录（可配置，见 uploadsDir）
    std::string filepath = uploadsDir() + "/" + filename;

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

// 创建预约表记录接口
