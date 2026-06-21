#include "personnelHandler.h"
#include "../user/userPhoneSync/userPhoneSync.h"
#include "../../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../../utils/roleTypeUtils/roleTypeUtils.h"

crow::response personnelHandler::createUser(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        std::string name = request_body.value("name", "");
        std::string phone = request_body.value("phone", "");
        std::string email = request_body.value("email", "");
        std::string password = request_body.value("password", "");
        std::string birthday = request_body.value("birthday", "1970-01-01");
        std::string head_image = request_body.value("head_image", "");

        if (name.empty())
        {
            return ResponseHelper::validation(req, "用户名不能为空");
        }

        if (phone.empty() && email.empty())
        {
            return ResponseHelper::validation(req, "手机号和邮箱至少填写一项");
        }

        if (password.empty())
        {
            password = "123456";
        }

        if (!phone.empty())
        {
            mysqlx::SqlResult phone_result = dbManager->getSession()
                                                 ->sql("SELECT user_id FROM phones WHERE phone = ?")
                                                 .bind(phone)
                                                 .execute();

            if (phone_result.count() > 0)
            {
                return ResponseHelper::validation(req, "手机号已存在");
            }
        }

        if (!email.empty())
        {
            mysqlx::SqlResult email_result = dbManager->getSession()
                                                 ->sql("SELECT id FROM users WHERE email = ?")
                                                 .bind(email)
                                                 .execute();

            if (email_result.count() > 0)
            {
                return ResponseHelper::validation(req, "邮箱已存在");
            }
        }

        const std::string hashed_password = hash_password(password);

        const int defaultUserRoleId =
            RoleTypeUtils::getRoleId(dbManager, "普通用户");
        if (defaultUserRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "普通用户角色不存在");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();

        mysqlx::SqlResult result;
        try
        {
            result = session->sql("INSERT INTO users (type_id, name, password, email, birthday, head_image) "
                                  "VALUES (?, ?, ?, ?, ?, ?)")
                         .bind(defaultUserRoleId, name, hashed_password, email, birthday, head_image)
                         .execute();

            if (result.getAffectedItemsCount() == 0)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::system_error(req, "创建失败");
            }

            if (!UserPhoneSync::upsertUserPhone(*session, static_cast<int>(result.getAutoIncrementValue()), phone))
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::system_error(req, "用户创建失败，手机号同步未完成");
            }

            session->sql("COMMIT").execute();
            AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
        }
        catch (...)
        {
            rollbackTransactionQuietly(*session);
            throw;
        }

        nlohmann::json payload;
        payload["success"] = true;
        payload["message"] = "创建成功";
        payload["data"] = {
            {"id", result.getAutoIncrementValue()},
            {"type_id", defaultUserRoleId},
            {"type_name", "普通用户"},
            {"name", name},
            {"phone", phone},
            {"email", email},
            {"birthday", birthday},
            {"head_image", head_image},
        };

        return ResponseHelper::created(req, payload);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}


crow::response personnelHandler::deleteUser(const crow::request &req, int &userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userID = request_body.value("user_id", 0);
        if (userID == 0)
        {
            return ResponseHelper::validation(req, "用户ID不能为空");
        }

        if (userID == userId)
        {
            return ResponseHelper::validation(req, "不能删除当前登录的超级管理员");
        }

        mysqlx::SqlResult target_result = dbManager->getSession()
                                              ->sql("SELECT type_id FROM users WHERE id = ? AND is_deleted = 0")
                                              .bind(userID)
                                              .execute();

        if (target_result.count() == 0)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        mysqlx::Row target_row = target_result.fetchOne();
        const int target_type = target_row[0].isNull() ? 0 : target_row[0].get<int>();
        const std::string target_role_name =
            RoleTypeUtils::getRoleName(dbManager, target_type);
        if (target_role_name != "普通用户")
        {
            return ResponseHelper::unavailable(req, "这里只能删除普通用户");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("UPDATE users "
                                             "SET is_deleted = 1, deleted_at = NOW(), deleted_by = ? "
                                             "WHERE id = ? AND type_id = ? AND is_deleted = 0")
                                       .bind(userId, userID, target_type)
                                       .execute();

        if (result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req, "用户不存在");
        }

        AdminHomeDataBroadcaster::instance().notifyHomeDataChanged();
        return ResponseHelper::success(req, "删除成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}


crow::response personnelHandler::createDoctor(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userId = request_body.value("user_id", 0);

        if (userId == 0)
        {
            return ResponseHelper::unavailable(req, "用户ID不能为空");
        }

        const int doctorRoleId = RoleTypeUtils::getRoleId(dbManager, "医生");
        if (doctorRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "医生角色不存在");
        }

        auto session = dbManager->getSession();
        session->sql("START TRANSACTION").execute();
        try
        {
            mysqlx::SqlResult result = session->sql("UPDATE users SET type_id = ? WHERE id = ?")
                                           .bind(doctorRoleId, userId)
                                           .execute();

            if (result.getAffectedItemsCount() == 0)
            {
                rollbackTransactionQuietly(*session);
                return ResponseHelper::notFound(req);
            }

            boost::posix_time::ptime currentDateTime = boost::posix_time::second_clock::local_time();
            std::string todayDate = formatDateOnly(currentDateTime);

            mysqlx::SqlResult onlineDoctorResult = session->sql("SELECT doctor_id FROM onlineDoctors WHERE doctor_id = ? LIMIT 1")
                                                       .bind(userId)
                                                       .execute();

            if (onlineDoctorResult.count() == 0)
            {
                mysqlx::SqlResult insertResult = session->sql("INSERT INTO onlineDoctors (doctor_id, date, check_in_time, check_out_time, status) "
                                                              "VALUES (?, ?, NULL, NULL, 'offline')")
                                                     .bind(userId)
                                                     .bind(todayDate)
                                                     .execute();
                if (insertResult.getAffectedItemsCount() != 1)
                {
                    rollbackTransactionQuietly(*session);
                    return ResponseHelper::operation_failed(req, "医生排班初始化失败");
                }
            }

            session->sql("COMMIT").execute();
        }
        catch (...)
        {
            rollbackTransactionQuietly(*session);
            throw;
        }

        return ResponseHelper::success(req, "给予权限成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}


crow::response personnelHandler::deleteDoctor(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userId = request_body.value("user_id", 0);

        if (userId == 0)
        {
            return ResponseHelper::unavailable(req, "用户ID不能为空");
        }

        const int defaultUserRoleId =
            RoleTypeUtils::getRoleId(dbManager, "普通用户");
        if (defaultUserRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "普通用户角色不存在");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("UPDATE users SET type_id = ? WHERE id = ?")
                                       .bind(defaultUserRoleId, userId)
                                       .execute();

        if (result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req);
        }

        return ResponseHelper::success(req, "删除权限成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}


crow::response personnelHandler::createWarehouserManager(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userId = request_body.value("user_id", 0);

        if (userId == 0)
        {
            return ResponseHelper::unavailable(req, "用户ID不能为空");
        }

        const int warehouseRoleId =
            RoleTypeUtils::getRoleId(dbManager, "仓库管理员");
        if (warehouseRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "仓库管理员角色不存在");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("UPDATE users SET type_id = ? WHERE id = ?")
                                       .bind(warehouseRoleId, userId)
                                       .execute();

        if (result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req);
        }

        return ResponseHelper::success(req, "给予权限成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}


crow::response personnelHandler::deleteWarehouserManager(const crow::request &req)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int userId = request_body.value("user_id", 0);

        if (userId == 0)
        {
            return ResponseHelper::unavailable(req, "用户ID不能为空");
        }

        const int defaultUserRoleId =
            RoleTypeUtils::getRoleId(dbManager, "普通用户");
        if (defaultUserRoleId <= 0)
        {
            return ResponseHelper::system_error(req, "普通用户角色不存在");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("UPDATE users SET type_id = ? WHERE id = ?")
                                       .bind(defaultUserRoleId, userId)
                                       .execute();

        if (result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::notFound(req);
        }

        return ResponseHelper::success(req, "删除权限成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

