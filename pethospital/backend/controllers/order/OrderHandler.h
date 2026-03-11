#ifndef ORDERHANDLER_H
#define ORDERHANDLER_H 

#include "../../utils/Utils.h"


class OrderHandler : public BaseHandler {
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;
    
public:
    explicit OrderHandler(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    crow::response createOrder(const crow::request& req);

    crow::response getOrderList(const crow::request &req, int& userId);                          // 获取订单列表 对应 /api/order/getOrderList

    crow::response getAllRecord(const crow::request& req, int &userId, int batch_size = 100, int offset = 1);                       //获取订单记录 对应 /api/order/getAllRecord
    
    nlohmann::json getOrderData(const int &orderId);
    crow::response getOrderInformation(const crow::request& req, int &orderId);    //获取订单信息 对应 /api/order/getOrderInformation/<int>

    crow::response changeOrder(const crow::request& req,int &orderId);             //修改订单 对应 /api/order/changeOrder/<int>

};

#endif
