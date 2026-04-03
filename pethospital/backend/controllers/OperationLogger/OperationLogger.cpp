#include "OperationLogger.h"

void OperationLogger::logSystemOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &module, const std::string &action, const std::string &result, const std::string &details, const std::string &source)
{
    try {
        if (!dbManager || !dbManager->getSession()) {
            return;
        }

        mysqlx::Session *session = dbManager->getSession();

        // 系统日志直接写入固定分类和来源，未细分角色时保留为空。
        session->sql("INSERT INTO system_operations "
                     "(category, operator, module, action, result, summary, details, source) "
                     "VALUES ('系统类', '系统', ?, ?, ?, 'system_operations', ?, ?)")
                .bind(module, action, result, details, source)
                .execute();
        std::cout <<  "系统操作日志记录成功" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "日志记录失败: " << e.what() << std::endl;
    }
}

void OperationLogger::logUserOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, int &userId, const std::string &module, const std::string &action, const std::string &result, const std::string &details, const std::string &source)
{
    try {
        if (!dbManager || !dbManager->getSession()) {
            return;
        }

        mysqlx::Session *session = dbManager->getSession();

        // 用户日志通过 users/types 联表补齐操作者和角色，保证返回给前端的数据结构完整。
        session->sql("INSERT INTO user_operations "
                     "(user_id, category, user_role, operator, module, action, result, summary, details, source) "
                     "SELECT u.id, '用户类', t.type, u.name, ?, ?, ?, 'user_operations', ?, ? "
                     "FROM users AS u "
                     "LEFT JOIN types AS t ON u.type_id = t.id "
                     "WHERE u.id = ?")
                .bind(module, action, result, details, source)
                .execute();
        std::cout << "用户操作日志记录成功" << std::endl;
    } catch (const std::exception& e)
    {
        std::cout << "日志记录失败: " << e.what() << std::endl;
    }
}
