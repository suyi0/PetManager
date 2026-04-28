#include "update.h"

void update::Automatic_update()
{
    boost::posix_time::ptime onlineDateTime = boost::posix_time::second_clock::local_time();
    // onlineDateTime.date()            -- 获取当前日期
    // onlineDateTime.time_of_day()     -- 获取当前时间

    // 更新员工工作时间记录
    boost::posix_time::time_duration cutoff_time(18, 30, 0); // 定义一个时间对象(18:30:00)
    if (onlineDateTime.time_of_day() >= cutoff_time)         // 判断当前时间是否超过18:30:00
    {
        updateWorkTimeRecord();
    }
}

void update::updateWorkTimeRecord(int batch_size, int offset)
{
    try
    {
        if (!checkDbConnection())
        {
            return;
        }

        mysqlx::Session *session = dbManager->getSession();

        int total_migrated = 0; // 记录已迁移的记录数
        while (true)
        {
            // 分批读取数据
            mysqlx::SqlResult result = session->sql(
                                                  "SELECT doctor_id, date, check_in_time, check_out_time, status "
                                                  "FROM onlineDoctors "
                                                  "WHERE date < CURDATE() AND status = 'offline' "
                                                  "LIMIT ? OFFSET ?")
                                           .bind(batch_size, (offset - 1) * batch_size)
                                           .execute();

            auto rows = result.fetchAll();
            if (result.count() == 0)
                break;

            session->sql("START TRANSACTION").execute();

            try
            {
                // 批量插入
                std::string insert_sql = "INSERT INTO workTimeRecords (doctor_id, date, check_in_time, check_out_time, status, notes) VALUES ";
                std::vector<std::string> values_list;

                for (const auto &row : rows)
                {
                    std::string values = "(" +
                                         std::to_string(row[0].get<int>()) + ", " +
                                         "'" + row[1].get<std::string>() + "', " +
                                         "'" + row[2].get<std::string>() + "', " +
                                         "'" + row[3].get<std::string>() + "', " +
                                         "'" + row[4].get<std::string>() + "', " +
                                         "'批量迁移'" +
                                         ")";
                    values_list.push_back(values);
                }

                // boost::join(values_list, ", ") - 将值列表用逗号连接成字符串
                insert_sql += boost::join(values_list, ", ");
                session->sql(insert_sql).execute();

                // 删除已处理的数据.
                std::string delete_sql = "DELETE FROM onlineDoctors WHERE (doctor_id, date) IN (";
                std::vector<std::string> delete_conditions;

                for (const auto &row : rows)
                {
                    delete_conditions.push_back("(" +
                                                std::to_string(row[0].get<int>()) + ", '" +
                                                row[1].get<std::string>() + "')");
                }

                delete_sql += boost::join(delete_conditions, ", ") + ")";
                session->sql(delete_sql).execute();

                session->sql("COMMIT").execute();

                int rows_count = 0;
                for (const auto &row : rows)
                {
                    rows_count++;
                }
                total_migrated += rows_count;
                offset++;
            }
            catch (const std::exception &e)
            {
                session->sql("ROLLBACK").execute();
                std::cerr << "Batch migration failed: " << e.what() << std::endl;
                break;
            }
        }

        std::cout << "Total migrated records: " << total_migrated << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Database operation failed: " << e.what() << std::endl;
    }
}
