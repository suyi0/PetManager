#include "OperationLogger.h"


void OperationLogger::logSystemOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, const std::string &operation, const std::string &details)
{
    try {
        mysqlx::Session *session = dbManager->getSession();
    
        session->sql("INSERT INTO system_operations (operation, details) VALUES (?, ?)")
                .bind(operation, details)
                .execute();
        std::cout << operation << "日志记录成功" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "日志记录失败: " << e.what() << std::endl;
    }
}

void OperationLogger::logUserOperation(std::shared_ptr<DatabaseManagerInterface> dbManager, int &userId, const std::string &operation, const std::string &details)
{
    try {
        mysqlx::Session *session = dbManager->getSession();
    
        session->sql("INSERT INTO user_operations (user_id, operation, details) VALUES (?, ?, ?)")
                .bind(userId, operation, details)
                .execute();
        std::cout << operation << "日志记录成功" << std::endl;
    } catch (const std::exception& e)
    {
        std::cout << "日志记录失败: " << e.what() << std::endl;
    }
}