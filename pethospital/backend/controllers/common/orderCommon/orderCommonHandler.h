#ifndef ORDERCOMMONHANDLER_H
#define ORDERCOMMONHANDLER_H 

#include "../../../utils/Utils.h"
#include "../../OperationLogger/OperationLogger.h"

class orderCommonHandler : public BaseHandler {
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:
    explicit orderCommonHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response getOrderSummary(const crow::request &req, int& userId);         // 获取订单列表
    crow::response getOrderRecords(const crow::request &req, int staffId);         // 获取医护端订单记录
    
    nlohmann::json getOrderData(const int &orderId);
    crow::response getOrderInformation(const crow::request& req, int &orderId);    //获取订单信息
};

#endif
