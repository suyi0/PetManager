#include "UserPhoneSync.h"

#include <iostream>

namespace UserPhoneSync
{
bool upsertUserPhone(DatabaseManagerInterface &dbManager, int user_id, const std::string &phone)
{
    if (user_id <= 0 || phone.empty())
    {
        return true;
    }

    try
    {
        auto *session = dbManager.getSession();
        auto result = session->sql("SELECT id FROM phones WHERE user_id = ? LIMIT 1")
                          .bind(user_id)
                          .execute();
        auto row = result.fetchOne();

        if (row)
        {
            session->sql("UPDATE phones SET phone = ? WHERE user_id = ?")
                .bind(phone, user_id)
                .execute();
        }
        else
        {
            session->sql("INSERT INTO phones (user_id, phone) VALUES (?, ?)")
                .bind(user_id, phone)
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
