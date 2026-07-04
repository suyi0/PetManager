#ifndef OPERATIONLOGGER_H
#define OPERATIONLOGGER_H

#include "../../utils/Utils.h"

class OperationLogger : public BaseHandler
{
private:
    std::shared_ptr<DatabaseManagerInterface> dbManager;

public:
    explicit OperationLogger(std::shared_ptr<DatabaseManagerInterface> db) : BaseHandler(db), dbManager(std::move(db)) {}

    static void logSystemOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &module, const std::string &details);
    static void logSystemOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &module, const std::string &action, const std::string &result, const std::string &summary = "", const std::string &details = "", const std::string &source = "");

    static void logUserOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, int userId, const std::string &module, const std::string &action, const std::string &result, const std::string &summary = "", const std::string &details = "", const std::string &source = "");

    
    static void FinalizeResponseWithOperationLog(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                                 const crow::request &req,
                                                 const crow::response &res,
                                                 const std::string &module,
                                                 const std::string &action,
                                                 std::optional<int> userId = std::nullopt);

    
    static void FinishLoggedRoute(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                  const crow::request &req,
                                  crow::response &res,
                                  const std::string &module,
                                  const std::string &action,
                                  std::optional<int> userId = std::nullopt,
                                  bool logSuccess = true);

    static void FinishSensitiveRoute(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                     const crow::request &req,
                                     crow::response &res,
                                     const std::string &module,
                                     const std::string &action,
                                     const std::string &permissionKey,
                                     std::optional<int> userId = std::nullopt);

    
    static void LogAuthorizationFailure(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                        const crow::request &req,
                                        const crow::response &res,
                                        const std::string &module,
                                        const std::string &action,
                                        std::optional<int> userId = std::nullopt);

    
    static void FinishAuthorizationFailure(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                           const crow::request &req,
                                           crow::response &res,
                                           const std::string &module,
                                           const std::string &action,
                                           std::optional<int> userId = std::nullopt);

    
    static void LogExceptionOperation(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                      const crow::request &req,
                                      const std::string &module,
                                      const std::string &action,
                                      const std::string &exceptionMessage,
                                      std::optional<int> userId = std::nullopt,
                                      const std::string &stage = "",
                                      const std::string &errorType = "");
};

#endif
