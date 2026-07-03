#include "warehouseManagerHandler.h"
#include "../../../services/redis/medicineRedisCache/MedicineRedisCache.h"
#include "../../../services/realtime/medicineBroadcaster/medicineStockBroadcaster.h"
#include "../../../utils/requestUtils/RequestUtils.h"

namespace
{
    // 跨文件复用的请求参数工具统一来自 RequestUtils（原本 finance/admin/warehouseManager 各复制一份）。
    // getQueryString / getQueryInt 仅本文件用到，按约定保留在本地。
    using RequestUtils::getJsonInt;
    using RequestUtils::getJsonString;
    using RequestUtils::normalizePage;
    using RequestUtils::normalizePageSize;

    std::string getQueryString(const crow::request &req, const std::string &key, const std::string &fallback = "")
    {
        const char *value = req.url_params.get(key);
        return value == nullptr ? fallback : std::string(value);
    }

    int getQueryInt(const crow::request &req, const std::string &key, int fallback)
    {
        const char *value = req.url_params.get(key);
        if (value == nullptr)
        {
            return fallback;
        }

        try
        {
            return std::stoi(value);
        }
        catch (const std::exception &)
        {
            return fallback;
        }
    }

    std::string resolveWarehouseOrderBy(const std::string &sortKey)
    {
        if (sortKey == "stock")
        {
            return "item_number DESC, id DESC";
        }
        if (sortKey == "price")
        {
            return "item_price DESC, id DESC";
        }
        if (sortKey == "total")
        {
            return "item_totalprice DESC, id DESC";
        }
        if (sortKey == "expiry")
        {
            return "item_expirationdate ASC, id DESC";
        }

        return "item_name ASC, id DESC";
    }

    nlohmann::json buildWarehouseItemJson(const mysqlx::Row &row)
    {
        nlohmann::json item_json;
        item_json["id"] = row[0].isNull() ? 0 : row[0].get<int>();
        item_json["item_name"] = row[1].isNull() ? "" : clean_string(row[1].get<std::string>());
        item_json["item_type"] = row[2].isNull() ? "" : row[2].get<std::string>();
        item_json["item_productiondate"] = row[3].isNull() ? "" : row[3].get<std::string>();
        item_json["item_expirationdate"] = row[4].isNull() ? "" : row[4].get<std::string>();
        item_json["days_until_expire"] = row[5].isNull() ? nullptr : nlohmann::json(row[5].get<int>());
        item_json["item_price"] = row[6].isNull() ? 0.0 : row[6].get<double>();
        item_json["item_number"] = row[7].isNull() ? 0 : row[7].get<int>();
        item_json["item_totalprice"] = row[8].isNull() ? 0.0 : row[8].get<double>();
        item_json["created_at"] = row[9].isNull() ? "" : row[9].get<std::string>();
        item_json["updated_at"] = row[10].isNull() ? "" : row[10].get<std::string>();
        return item_json;
    }
}

crow::response warehouseManagerHandler::upload(const crow::request &req)
{
    crow::response res;
    auto request_body_opt = validateRequest(req, res);
    if (!request_body_opt)
        return res;
    auto &request_body = request_body_opt.value();

    try
    {
        if(request_body["item_name"].is_null()
            || request_body["item_type"].is_null()
            || request_body["item_productiondate"].is_null()
            || request_body["item_expirationdate"].is_null()
            || request_body["item_price"].is_null()
            || request_body["item_number"].is_null()
        )
        {
            return ResponseHelper::validation(req, "上传数据不完整");
        }

        std::string item_name = request_body["item_name"].get<std::string>();
        std::string item_type = request_body["item_type"].get<std::string>();
        std::string item_productiondate = request_body["item_productiondate"].get<std::string>();
        std::string item_expirationdate = request_body["item_expirationdate"].get<std::string>();
        double item_price = request_body["item_price"].get<double>();
        int item_number = request_body["item_number"].get<int>();

        if(item_name.empty()
            || item_type.empty()
            || item_productiondate.empty()
            || item_expirationdate.empty())
        {
            return ResponseHelper::validation(req, "上传数据不完整");
        }

        if(item_price <= 0.0 || item_number <= 0)
        {
            return ResponseHelper::validation(req, "价格和数量必须大于0");
        }

        if(item_productiondate > item_expirationdate)
        {
            return ResponseHelper::validation(req, "生产日期不能晚于过期日期");
        }

        mysqlx::RowResult result = dbManager->getSession()->sql("INSERT INTO warehouse (item_name, item_type, item_productiondate, item_expirationdate, item_price, item_number) VALUES (?, ?, ?, ?, ?, ?)")
                                .bind(item_name, item_type, item_productiondate, item_expirationdate, item_price, item_number)
                                .execute();
                                
        if(result.getAffectedItemsCount() > 0)
        {
            MedicineRedisCache::invalidateMedicineCache();
            MedicineStockBroadcaster::instance().notifyMedicineStockChanged();
            return ResponseHelper::success(req, "上传数据成功");
        }
        else
        {
            return ResponseHelper::system_error(req, "数据上传失败");
        }
    }
    catch(const std::exception& e)
    {
        return ResponseHelper::system_error(req);
    }
}

crow::response warehouseManagerHandler::selectAllData(const crow::request &req)
{
    try
    {
        if(!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        const bool hasListParams =
            req.url_params.get("keyword") != nullptr ||
            req.url_params.get("itemType") != nullptr ||
            req.url_params.get("sortKey") != nullptr ||
            req.url_params.get("page") != nullptr ||
            req.url_params.get("pageSize") != nullptr;

        if (hasListParams)
        {
            const std::string keyword = getQueryString(req, "keyword");
            const std::string itemType = getQueryString(req, "itemType", "全部");
            const std::string sortKey = getQueryString(req, "sortKey", "name");
            const std::string likeKeyword = "%" + keyword + "%";
            const int page = normalizePage(getQueryInt(req, "page", 1));
            const int pageSize = normalizePageSize(getQueryInt(req, "pageSize", 10), 10, 100);
            const int offset = (page - 1) * pageSize;
            const bool filterType = !itemType.empty() && itemType != "全部";
            const std::string typeCondition = filterType ? "AND item_type = ? " : "";

            auto query = dbManager->getSession()
                             ->sql(std::string("SELECT id, item_name, item_type, CAST(item_productiondate AS CHAR), CAST(item_expirationdate AS CHAR), "
                                               "days_until_expire, item_price, item_number, item_totalprice, CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
                                               "FROM warehouse "
                                               "WHERE is_deleted = 0 "
                                               "AND (? = '' OR item_name LIKE ?) ") +
                                   typeCondition +
                                   "ORDER BY " + resolveWarehouseOrderBy(sortKey) + " "
                                   "LIMIT ?, ?")
                             .bind(keyword, likeKeyword);
            if (filterType)
            {
                query.bind(itemType);
            }
            query.bind(offset, pageSize);
            mysqlx::SqlResult result = query.execute();

            auto countQuery = dbManager->getSession()
                                  ->sql(std::string("SELECT COUNT(*) "
                                                    "FROM warehouse "
                                                    "WHERE is_deleted = 0 "
                                                    "AND (? = '' OR item_name LIKE ?) ") +
                                        typeCondition)
                                  .bind(keyword, likeKeyword);
            if (filterType)
            {
                countQuery.bind(itemType);
            }
            mysqlx::SqlResult countResult = countQuery.execute();

            nlohmann::json items = nlohmann::json::array();
            for (auto row : result)
            {
                items.push_back(buildWarehouseItemJson(row));
            }

            nlohmann::json data = {
                {"items", items},
                {"total", countResult.fetchOne()[0].get<int>()},
                {"page", page},
                {"pageSize", pageSize}};

            return ResponseHelper::success(req, data);
        }

        mysqlx::SqlResult result = dbManager->getSession()->sql(
            "SELECT id, item_name, item_type, CAST(item_productiondate AS CHAR), CAST(item_expirationdate AS CHAR), "
            "days_until_expire, item_price, item_number, item_totalprice, CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
            "FROM warehouse WHERE is_deleted = 0 ORDER BY id DESC")
            .execute();

        nlohmann::json response_data = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json item_json;
            item_json["id"] = row[0].isNull() ? 0 : row[0].get<int>();
            item_json["item_name"] = row[1].isNull() ? "" : clean_string(row[1].get<std::string>());
            item_json["item_type"] = row[2].isNull() ? "" : row[2].get<std::string>();
            item_json["item_productiondate"] = row[3].isNull() ? "" : row[3].get<std::string>();
            item_json["item_expirationdate"] = row[4].isNull() ? "" : row[4].get<std::string>();
            item_json["days_until_expire"] = row[5].isNull() ? nullptr : nlohmann::json(row[5].get<int>());
            item_json["item_price"] = row[6].isNull() ? 0.0 : row[6].get<double>();
            item_json["item_number"] = row[7].isNull() ? 0 : row[7].get<int>();
            item_json["item_totalprice"] = row[8].isNull() ? 0.0 : row[8].get<double>();
            item_json["created_at"] = row[9].isNull() ? "" : row[9].get<std::string>();
            item_json["updated_at"] = row[10].isNull() ? "" : row[10].get<std::string>();
            response_data.push_back(item_json);
        }

        return ResponseHelper::success(req, response_data);
    }
    catch(const std::exception& e)
    {
        return ResponseHelper::system_error(req, "获取仓库数据失败: " + std::string(e.what()));
    }
}

crow::response warehouseManagerHandler::selectData(const crow::request &req, const std::string& identifier, const std::string& value)
{
    try
    {
        if(!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        mysqlx::SqlResult result;

        if(identifier == "data-id")
        {
            int dataID = 0;
            try
            {
                dataID = std::stoi(value);
            }
            catch (const std::exception&)
            {
                return ResponseHelper::validation(req, "dataID 必须是有效数字");
            }

            if(dataID <= 0)
            {
                return ResponseHelper::validation(req, "无效的数据ID");
            }

            result = dbManager->getSession()->sql(
                "SELECT id, item_name, item_type, CAST(item_productiondate AS CHAR), CAST(item_expirationdate AS CHAR), "
                "days_until_expire, item_price, item_number, item_totalprice, CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
                "FROM warehouse WHERE id = ? AND is_deleted = 0")
                .bind(dataID)
                .execute();
        }
        else if(identifier == "item-name")
        {
            if(value.empty())
            {
                return ResponseHelper::validation(req, "item_name 不能为空");
            }

            result = dbManager->getSession()->sql(
                "SELECT id, item_name, item_type, CAST(item_productiondate AS CHAR), CAST(item_expirationdate AS CHAR), "
                "days_until_expire, item_price, item_number, item_totalprice, CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
                "FROM warehouse WHERE item_name = ? AND is_deleted = 0 ORDER BY id DESC")
                .bind(value)
                .execute();
        }
        else
        {
            return ResponseHelper::validation(req, "identifier 仅支持 data-id 或 item-name");
        }

        nlohmann::json response_data = nlohmann::json::array();
        for (auto row : result)
        {
            nlohmann::json item_json;
            item_json["id"] = row[0].isNull() ? 0 : row[0].get<int>();
            item_json["item_name"] = row[1].isNull() ? "" : clean_string(row[1].get<std::string>());
            item_json["item_type"] = row[2].isNull() ? "" : row[2].get<std::string>();
            item_json["item_productiondate"] = row[3].isNull() ? "" : row[3].get<std::string>();
            item_json["item_expirationdate"] = row[4].isNull() ? "" : row[4].get<std::string>();
            item_json["days_until_expire"] = row[5].isNull() ? nullptr : nlohmann::json(row[5].get<int>());
            item_json["item_price"] = row[6].isNull() ? 0.0 : row[6].get<double>();
            item_json["item_number"] = row[7].isNull() ? 0 : row[7].get<int>();
            item_json["item_totalprice"] = row[8].isNull() ? 0.0 : row[8].get<double>();
            item_json["created_at"] = row[9].isNull() ? "" : row[9].get<std::string>();
            item_json["updated_at"] = row[10].isNull() ? "" : row[10].get<std::string>();
            response_data.push_back(item_json);
        }

        if(response_data.empty())
        {
            return ResponseHelper::notFound(req, "未找到对应的仓库数据");
        }

        if(identifier == "data-id")
        {
            return ResponseHelper::success(req, response_data[0]);
        }

        return ResponseHelper::success(req, response_data);
    }
    catch(const std::exception& e)
    {
        return ResponseHelper::system_error(req, "获取仓库数据失败: " + std::string(e.what()));
    }
}

crow::response warehouseManagerHandler::searchItems(const crow::request &req, const nlohmann::json &requestBody)
{
    try
    {
        if(!checkDbConnection())
        {
            return ResponseHelper::database_error(req, "Database connection failed", "无法连接到数据库");
        }

        const std::string keyword = getJsonString(requestBody, "keyword");
        const std::string itemType = getJsonString(requestBody, "itemType", "全部");
        const std::string sortKey = getJsonString(requestBody, "sortKey", "name");
        const std::string likeKeyword = "%" + keyword + "%";
        const int page = normalizePage(getJsonInt(requestBody, "page", 1));
        const int pageSize = normalizePageSize(getJsonInt(requestBody, "pageSize", 10), 10, 100);
        const int offset = (page - 1) * pageSize;
        const bool filterType = !itemType.empty() && itemType != "全部";
        const std::string typeCondition = filterType ? "AND item_type = ? " : "";

        auto query = dbManager->getSession()
                         ->sql(std::string("SELECT id, item_name, item_type, CAST(item_productiondate AS CHAR), CAST(item_expirationdate AS CHAR), "
                                           "days_until_expire, item_price, item_number, item_totalprice, CAST(created_at AS CHAR), CAST(updated_at AS CHAR) "
                                           "FROM warehouse "
                                           "WHERE is_deleted = 0 "
                                           "AND (? = '' OR item_name LIKE ?) ") +
                               typeCondition +
                               "ORDER BY " + resolveWarehouseOrderBy(sortKey) + " "
                               "LIMIT ?, ?")
                         .bind(keyword, likeKeyword);
        if (filterType)
        {
            query.bind(itemType);
        }
        query.bind(offset, pageSize);
        mysqlx::SqlResult result = query.execute();

        auto countQuery = dbManager->getSession()
                              ->sql(std::string("SELECT COUNT(*) "
                                                "FROM warehouse "
                                                "WHERE is_deleted = 0 "
                                                "AND (? = '' OR item_name LIKE ?) ") +
                                    typeCondition)
                              .bind(keyword, likeKeyword);
        if (filterType)
        {
            countQuery.bind(itemType);
        }
        mysqlx::SqlResult countResult = countQuery.execute();

        nlohmann::json items = nlohmann::json::array();
        for (auto row : result)
        {
            items.push_back(buildWarehouseItemJson(row));
        }

        nlohmann::json data = {
            {"items", items},
            {"total", countResult.fetchOne()[0].get<int>()},
            {"page", page},
            {"pageSize", pageSize}};

        return ResponseHelper::success(req, data);
    }
    catch(const std::exception& e)
    {
        return ResponseHelper::system_error(req, "搜索仓库数据失败: " + std::string(e.what()));
    }
}

crow::response warehouseManagerHandler::updata(const crow::request &req, const int& dataID)
{
    crow::response res;
    auto request_body_opt = validateRequest(req, res);
    if (!request_body_opt)
        return res;
    auto &request_body = request_body_opt.value();

    try
    {
        if(request_body["id"].is_null()
            || request_body["item_name"].is_null()
            || request_body["item_type"].is_null()
            || request_body["item_productiondate"].is_null()
            || request_body["item_expirationdate"].is_null()
            || request_body["item_price"].is_null()
            || request_body["item_number"].is_null())
        {
            return ResponseHelper::validation(req, "更新数据不完整");
        }

        std::string item_name = request_body["item_name"].get<std::string>();
        std::string item_type = request_body["item_type"].get<std::string>();
        std::string item_productiondate = request_body["item_productiondate"].get<std::string>();
        std::string item_expirationdate = request_body["item_expirationdate"].get<std::string>();
        int item_number = request_body["item_number"].get<int>();
        double item_price = request_body["item_price"].get<double>();

        if(dataID <= 0)
        {
            return ResponseHelper::validation(req, "无效的数据ID");
        }

        if(item_name.empty()
            || item_type.empty()
            || item_productiondate.empty()
            || item_expirationdate.empty())
        {
            return ResponseHelper::validation(req, "更新数据不完整");
        }

        if(item_price <= 0.0 || item_number < 0)
        {
            return ResponseHelper::validation(req, "价格必须大于0，数量不能小于0");
        }

        if(item_productiondate > item_expirationdate)
        {
            return ResponseHelper::validation(req, "生产日期不能晚于过期日期");
        }

        mysqlx::RowResult result = dbManager->getSession()->sql(
            "UPDATE warehouse SET item_name = ?, item_type = ?, item_productiondate = ?, item_expirationdate = ?, item_number = ?, item_price = ? "
            "WHERE id = ? AND is_deleted = 0")
                .bind(item_name, item_type, item_productiondate, item_expirationdate, item_number, item_price, dataID)
                .execute();

        if(result.getAffectedItemsCount() > 0)
        {
            MedicineRedisCache::invalidateMedicineCache();
            MedicineStockBroadcaster::instance().notifyMedicineStockChanged();
            return ResponseHelper::success(req, "库存更新成功");
        }

        return ResponseHelper::notFound(req, "未找到要更新的数据");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req);
    }
}

crow::response warehouseManagerHandler::deleteData(const crow::request &req, int userId)
{
    try
    {
        crow::response res;
        auto request_body_opt = validateRequest(req, res);
        if (!request_body_opt)
            return res;
        auto &request_body = request_body_opt.value();

        int dataID = request_body.contains("dataID") ? request_body["dataID"].get<int>() : -1;

        if(dataID <= 0)
        {
            return ResponseHelper::validation(req, "无效的数据ID");
        }

        mysqlx::RowResult result = dbManager->getSession()
                                    ->sql("UPDATE warehouse "
                                          "SET is_deleted = 1, deleted_at = NOW(), deleted_by = ? "
                                          "WHERE id = ? AND is_deleted = 0")
                                    .bind(userId, dataID)
                                    .execute();
        
        if(result.getAffectedItemsCount() > 0)
        {
            MedicineRedisCache::invalidateMedicineCache();
            MedicineStockBroadcaster::instance().notifyMedicineStockChanged();
            return ResponseHelper::success(req, "删除数据成功");
        }
        else
        {
            return ResponseHelper::notFound(req, "未找到要删除的数据");
        }
    }
    catch(const std::exception& e)
    {
        return ResponseHelper::system_error(req);
    }
}
