#include "userPhoneSync.h"

#include <iostream>

namespace UserPhoneSync
{
    // 实行手机号同步操作函数
    bool upsertUserPhone(mysqlx::Session &session, int userId, const std::string &phone)
    {
        if (userId <= 0 || phone.empty())
        {
            return true;
        }

        try
        {
            auto result = session.sql("SELECT id FROM phones WHERE user_id = ? LIMIT 1")
                              .bind(userId)
                              .execute();
            auto row = result.fetchOne();

            if (row)
            {
                session.sql("UPDATE phones SET phone = ? WHERE user_id = ?")
                    .bind(phone, userId)
                    .execute();
            }
            else
            {
                session.sql("INSERT INTO phones (user_id, phone) VALUES (?, ?)")
                    .bind(userId, phone)
                    .execute();
            }

            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to sync users.phone into phones: " << e.what() << std::endl;
            return false;
        }
    }
}
