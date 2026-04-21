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

    // 将响应解析与日志落库组合为统一收口，适合在路由返回前直接调用。
    static void FinalizeResponseWithOperationLog(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                                 const crow::request &req,
                                                 const crow::response &res,
                                                 const std::string &module,
                                                 const std::string &action,
                                                 std::optional<int> userId = std::nullopt);

    // 在统一收口里同时处理“是否记录成功日志”和最终 res.end()，避免各路由复制同一段模板代码。
    static void FinishLoggedRoute(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                  const crow::request &req,
                                  crow::response &res,
                                  const std::string &module,
                                  const std::string &action,
                                  std::optional<int> userId = std::nullopt,
                                  bool logSuccess = true);

    // 统一记录令牌无效、权限不足等鉴权失败事件，和普通结果日志区分开。
    static void LogAuthorizationFailure(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                        const crow::request &req,
                                        const crow::response &res,
                                        const std::string &module,
                                        const std::string &action,
                                        std::optional<int> userId = std::nullopt);

    // 鉴权失败场景只记录安全日志，不再重复记录普通失败结果日志。
    static void FinishAuthorizationFailure(std::shared_ptr<DatabaseManagerInterface> dbManager,
                                           const crow::request &req,
                                           crow::response &res,
                                           const std::string &module,
                                           const std::string &action,
                                           std::optional<int> userId = std::nullopt);

    // 统一封装异常日志记录，提供阶段和类型参数方便后续检索和统计。                                             
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
