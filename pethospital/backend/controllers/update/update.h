#ifndef UPDATE_H
#define UPDATE_H

#include "../../utils/Utils.h"
#include <boost/algorithm/string/join.hpp>

class update : public BaseHandler {
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit update(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db) {}

    void Automatic_update();                                                        // 系统自动更新操作

    void updateWorkTimeRecord(int batch_size = 100, int offset = 1);                // 批量更新（一批更新100条数据）
};

#endif