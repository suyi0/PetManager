#include "warehouseHandler.h"

crow::response warehouseHandler::updataWarehouse(const crow::request &req)
{
    crow::response res;
    auto request_body_opt = validateRequest(req, res);
    if (!request_body_opt)
        return res;
    auto &request_body = request_body_opt.value();

    try
    {
        std::string item_name = request_body["item_name"].is_null() ? "" : request_body["item_name"].get<std::string>();
        std::string item_type = request_body["item_type"].is_null() ? "" : request_body["item_type"].get<std::string>();
        std::string item_productiondate = request_body["item_productiondate"].is_null() ? "" : request_body["item_productiondate"].get<std::string>();
        std::string item_expirationdate = request_body["item_expirationdate"].is_null() ? "" : request_body["item_expirationdate"].get<std::string>();
        int item_number = request_body["item_number"].is_null() ? 0 : request_body["item_number"].get<int>();
        double item_price = request_body["item_price"].is_null() ? 0.00 : request_body["item_price"].get<double>();
        double item_totalprice = item_number * item_price;


        if(!item_name.empty() && !item_type.empty() && !item_productiondate.empty() && !item_expirationdate.empty() && item_price > 0.00 && item_totalprice > 0.00)
        {
            dbManager->getSession()->sql("INSERT INTO warehouse (item_name, item_type, item_productiondate, item_expirationdate, item_number, item_price, item_totalprice) VALUES (?, ?, ?, ?, ?, ?, ?)")
                                .bind(item_name, item_type, item_productiondate, item_expirationdate, item_number, item_price, item_totalprice)
                                .execute();
        }


        return ResponseHelper::success(req, "库存更新成功");
    }
    catch (const std::exception &e)
    {
        return ResponseHelper::system_error(req);
    }
}