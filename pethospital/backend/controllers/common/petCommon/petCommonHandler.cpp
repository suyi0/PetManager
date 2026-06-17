#include "petCommonHandler.h"

namespace
{
    nlohmann::json mapPetRow(const mysqlx::Row &row)
    {
        // lambda函数用于安全地获取字符串值，处理NULL情况
        const auto safeString = [&row](int index) -> std::string
        {
            if (row[index].isNull())
            {
                return "";
            }

            return clean_string(row[index].get<std::string>());
        };

        return {
            {"id", std::to_string(row[0].get<int>())},
            {"name", safeString(1)},
            {"species", safeString(2)},
            {"breed", safeString(5)},
            {"age", safeString(3)},
            {"gender", safeString(4)},
            {"neutered", safeString(6)},
            {"vaccineStatus", safeString(7)},
            {"preference", safeString(8)},
            {"notes", safeString(9)},
        };
    }
}

crow::response petCommonHandler::createPetProfile(const crow::request &req, int userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
        {
            return res;
        }
        auto &request_body = request_body_opt.value();

        const std::string name = getRequestString(request_body, "name");
        if (name.empty())
        {
            return ResponseHelper::validation(req, "宠物名称不能为空");
        }

        mysqlx::SqlResult createResult = dbManager->getSession()
                                             ->sql("INSERT INTO pets "
                                                   "(user_id, pet_name, pet_type, pet_age, pet_sex, pet_breed, pet_neutered, vaccine_status, preference, notes) "
                                                   "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
                                             .bind(userId)
                                             .bind(name)
                                             .bind(getRequestString(request_body, "species"))
                                             .bind(getRequestString(request_body, "age"))
                                             .bind(getRequestString(request_body, "gender"))
                                             .bind(getRequestString(request_body, "breed"))
                                             .bind(getRequestString(request_body, "neutered"))
                                             .bind(getRequestString(request_body, "vaccineStatus"))
                                             .bind(getRequestString(request_body, "preference"))
                                             .bind(getRequestString(request_body, "notes"))
                                             .execute();

        const int petId = static_cast<int>(createResult.getAutoIncrementValue());
        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT id, COALESCE(pet_name, ''), COALESCE(pet_type, ''), "
                                             "COALESCE(pet_age, ''), COALESCE(pet_sex, ''), COALESCE(pet_breed, ''), "
                                             "COALESCE(pet_neutered, ''), COALESCE(vaccine_status, ''), "
                                             "COALESCE(preference, ''), COALESCE(notes, '') "
                                             "FROM pets WHERE id = ? AND user_id = ? AND is_deleted = 0")
                                       .bind(petId, userId)
                                       .execute();

        nlohmann::json response = nlohmann::json::array();
        for (auto row : result)
        {
            response.push_back(mapPetRow(row));
        }

        if (response.empty())
        {
            return ResponseHelper::notFound(req, "Pet profile not found");
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response petCommonHandler::getPetProfiles(const crow::request &req, int userId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT id, COALESCE(pet_name, ''), COALESCE(pet_type, ''), "
                                             "COALESCE(pet_age, ''), COALESCE(pet_sex, ''), COALESCE(pet_breed, ''), "
                                             "COALESCE(pet_neutered, ''), COALESCE(vaccine_status, ''), "
                                             "COALESCE(preference, ''), COALESCE(notes, '') "
                                             "FROM pets WHERE user_id = ? AND is_deleted = 0 ORDER BY id DESC")
                                       .bind(userId)
                                       .execute();

        nlohmann::json response = nlohmann::json::array();
        for (auto row : result)
        {
            response.push_back(mapPetRow(row));
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response petCommonHandler::updatePetProfile(const crow::request &req, int userId, int petId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
        {
            return res;
        }
        auto &request_body = request_body_opt.value();

        const std::string name = getRequestString(request_body, "name");
        if (name.empty())
        {
            return ResponseHelper::validation(req, "宠物名称不能为空");
        }

        dbManager->getSession()
            ->sql("UPDATE pets SET pet_name = ?, pet_type = ?, pet_age = ?, pet_sex = ?, pet_breed = ?, "
                  "pet_neutered = ?, vaccine_status = ?, preference = ?, notes = ? "
                  "WHERE id = ? AND user_id = ? AND is_deleted = 0")
            .bind(name)
            .bind(getRequestString(request_body, "species"))
            .bind(getRequestString(request_body, "age"))
            .bind(getRequestString(request_body, "gender"))
            .bind(getRequestString(request_body, "breed"))
            .bind(getRequestString(request_body, "neutered"))
            .bind(getRequestString(request_body, "vaccineStatus"))
            .bind(getRequestString(request_body, "preference"))
            .bind(getRequestString(request_body, "notes"))
            .bind(petId, userId)
            .execute();

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("SELECT id, COALESCE(pet_name, ''), COALESCE(pet_type, ''), "
                                             "COALESCE(pet_age, ''), COALESCE(pet_sex, ''), COALESCE(pet_breed, ''), "
                                             "COALESCE(pet_neutered, ''), COALESCE(vaccine_status, ''), "
                                             "COALESCE(preference, ''), COALESCE(notes, '') "
                                             "FROM pets WHERE id = ? AND user_id = ? AND is_deleted = 0")
                                       .bind(petId, userId)
                                       .execute();

        nlohmann::json response = nlohmann::json::array();
        for (auto row : result)
        {
            response.push_back(mapPetRow(row));
        }

        if (response.empty())
        {
            return ResponseHelper::notFound(req, "Pet profile not found");
        }

        return ResponseHelper::success(req, response);
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}

crow::response petCommonHandler::deletePetProfile(const crow::request &req, int userId, int petId)
{
    try
    {
        if (!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        mysqlx::SqlResult result = dbManager->getSession()
                                       ->sql("UPDATE pets "
                                             "SET is_deleted = 1, deleted_at = NOW(), deleted_by = ? "
                                             "WHERE id = ? AND user_id = ? AND is_deleted = 0")
                                       .bind(userId, petId, userId)
                                       .execute();
        
        if(result.getAffectedItemsCount() == 0)
        {
            return ResponseHelper::error(req, "宠物档案删除失败");
        }

        return ResponseHelper::success(req, "宠物档案已删除");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req, e.what());
    }
}
