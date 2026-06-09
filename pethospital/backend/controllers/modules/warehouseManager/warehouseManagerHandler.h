#ifndef WAREHOUSEMANAGERHANDLER_H
#define WAREHOUSEMANAGERHANDLER_H 

#include "../../../utils/Utils.h"

class warehouseManagerHandler : public BaseHandler {
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:
    explicit warehouseManagerHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response upload(const crow::request& req); // 上传仓库资源，对应 /api/warehouse-managers/items
    
    crow::response selectAllData(const crow::request& req); // 查询仓库信息，对应 /api/warehouse-managers/items
    
    crow::response selectData(const crow::request& req, const std::string& identifier, const std::string& value); // 按 data-id 或 item-name 查询仓库信息，对应 /api/warehouse-managers/items/<identifier>/<value>
    
    crow::response updata(const crow::request& req, const int& dataID); // 修改仓库信息，对应 /api/warehouse-managers/items/<int>

    crow::response deleteData(const crow::request& req, int userId); // 软删除仓库资源，对应 /api/warehouse-managers/item-deletions
};

#endif
