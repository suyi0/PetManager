#ifndef ORDERCOMMONHANDLER_H
#define ORDERCOMMONHANDLER_H 

#include "../../../utils/Utils.h"
#include "../../OperationLogger/OperationLogger.h"

class orderCommonHandler : public BaseHandler {
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
public:
    explicit orderCommonHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response getOrderSummary(const crow::request &req, int& userId);         // 获取订单摘要，对应 /api/user/orders 与 /api/doctor/order/getOrderSummary
    
    nlohmann::json getOrderData(const int &orderId);                               // 订单详情内部组装函数
    crow::response getOrderInformation(const crow::request& req, int &orderId);    // 获取订单详情，对应 /api/user/orders/<int> 与 /api/doctor/order/getOrderInformation/<int>
};

#endif
