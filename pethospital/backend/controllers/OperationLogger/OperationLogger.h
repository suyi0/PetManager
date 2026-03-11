#ifndef OPERATIONLOGGER_H
#define OPERATIONLOGGER_H

#include "../../utils/Utils.h"

class OperationLogger : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit OperationLogger(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    static void logSystemOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &operation, const std::string &details = "");
 
    static void logUserOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, int &userId, const std::string &operation, const std::string &details = "");
};

#endif
