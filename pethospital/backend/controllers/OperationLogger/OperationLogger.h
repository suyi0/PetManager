#ifndef OPERATIONLOGGER_H
#define OPERATIONLOGGER_H

#include "../../utils/Utils.h"

class OperationLogger : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit OperationLogger(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    static void logSystemOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &module, const std::string &action, const std::string &result, const std::string &details = "", const std::string &source);
 
    static void logUserOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, int &userId, const std::string &module, const std::string &action, const std::string &result, const std::string &details = "", const std::string &source);
};

#endif
