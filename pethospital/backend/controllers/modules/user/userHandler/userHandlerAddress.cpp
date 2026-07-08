#include "userHandler.h"
#include "../../../../utils/AuthIdentifierUtils.h"
#include "../userPhoneSync/userPhoneSync.h"
#include "../../../../services/auth/AuthSessionStore.h"
#include "../../../../services/auth/AuthLoginFailureStore.h"
#include "../../../../services/redis/RedisClient.h"
#include "../../../../services/redis/redisLock/RedisLock.h"
#include "../../../../services/redis/doctorListCache/DoctorListCache.h"
#include "../../../../utils/requestUtils/RequestUtils.h"
#include "../../../../services/realtime/adminBroadcaster/adminHomeDataBroadcaster.h"
#include "../../../../services/realtime/doctorBroadcaster/doctorQueueBroadcaster.h"
#include "../../../../services/realtime/doctorListBroadcaster/doctorListBroadcaster.h"
#include "statusLabelUtils/StatusLabelUtils.h"
#include <vector>

// userHandler 地址域：新增/更新收货地址（含地理编码）。
// 从 userHandler.cpp 按域拆出；类声明仍在 userHandler.h。

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
                rollbackTransactionQuietly(*session);
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
            rollbackTransactionQuietly(*session);
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
                rollbackTransactionQuietly(*session);
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
                rollbackTransactionQuietly(*session);
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
            rollbackTransactionQuietly(*session);
            throw;
        }
    }
    catch (const std::exception &e)
    {
        OperationLogger::LogExceptionOperation(dbManager, req, "地址", "更新地址", "Failed to update address with ID " + std::to_string(addressId) + ": " + std::string(e.what()));
        return ResponseHelper::operation_failed(req, "Failed to update address", e.what());
    }
}

