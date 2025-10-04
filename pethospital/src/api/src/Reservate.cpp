#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mysqlx/xdevapi.h>

#include "User.cpp"

extern mysqlx::Session *g_db_session;
extern mysqlx::Schema *g_database;

class Reservate
{
public:
    Reservate();

    std::string date();

    void setBefore(const std::string &time)
    {
        this->time_before = time;
    }
    std::string getBefore()
    {
        return this->time_before;
    }
    void setEnd(const std::string &time)
    {
        this->time_end = time;
    }
    std::string getEnd()
    {
        return this->time_end;
    }

    std::string addTime(const std::string &time);
    void setReservate_time(const std::string &date, const std::string &time)
    {
        reservate_time.insert({date, time});
    }

private:
    std::string name;
    std::string time_before;
    std::string time_end;
    std::multimap<std::string, std::string> reservate_time;
};

Reservate::Reservate()
{
    this->time_before = "09:00";
    this->time_end = "10:00";
}
std::string Reservate::date()
{
    std::time_t t = std::time(nullptr);
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&t));
    return std::string(buffer);
};

std::string Reservate::addTime(const std::string &time)
{
    std::string timeStr = time;                   // 这里直接使用传入的 time 参数
    int hour = std::stoi(timeStr.substr(0, 2));   // 提取小时部分
    int minute = std::stoi(timeStr.substr(3, 2)); // 提取分钟部分

    hour += 1; // 将小时数加1

    // 格式化回时间字符串，确保两位数显示
    std::string newTime;
    newTime = (hour < 10 ? "0" : "") + std::to_string(hour) + ":" +
              (minute < 10 ? "0" : "") + std::to_string(minute);
    return newTime;
}

int main()
{
    // 获取医生数据
    User u;
    std::string email = "";
    std::string phone = "";
    std::string password = "";
    try
    {
        mysqlx::Table users_table = g_database->getTable("doctors");
        mysqlx::RowResult result;
        if (!email.empty())
        {
            // 通过email查询用户
            result = users_table.select("id", "name", "password", "phone", "email", "CAST(birthday AS CHAR)", "creation_time", "address_id", "head_image")
                         .where("email = :email")
                         .bind("email", email)
                         .execute();
        }
        else if (!phone.empty())
        {
            // 通过phone查询用户
            result = users_table.select("id", "name", "password", "phone", "email", "CAST(birthday AS CHAR)", "creation_time", "address_id", "head_image")
                         .where("phone = :phone")
                         .bind("phone", phone)
                         .execute();
        }

        for (auto row : result)
        {
            u.id = row[0].get<int>();
            u.name = clean_string(row[1].get<std::string>());
            u.email = clean_string(row[2].get<std::string>());
            u.phone = clean_string(row[3].get<std::string>());
            // 修复birthday字段赋值，从字符串转换为boost::gregorian::date类型
            std::string birthday_str = clean_string(row[4].get<std::string>());
            if (!birthday_str.empty())
            {
                u.birthday = boost::gregorian::from_simple_string(birthday_str);
            }
            else
            {
                u.birthday = boost::gregorian::date(1970, 1, 1);
            }
            u.address_id = clean_string(row[5].get<std::string>());
            u.head_image = clean_string(row[6].get<std::string>());
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error fetching doctor data: " << e.what() << std::endl;
        return -1;
    }
    if (u.token == true)
    {
        Reservate r;
        if (std::stoi(r.getEnd().substr(0, 2)) <= 12)
        {
            for (int i = 0; i < 3; i++)
            {
                r.setReservate_time(r.date(), (r.getBefore() + "-" + r.getEnd()));
                r.setBefore(r.addTime(r.getBefore()));
                r.setEnd(r.addTime(r.getEnd()));
            }
            r.setBefore("14:30");
            r.setEnd("15:30");
        }
        else if (std::stoi(r.getEnd().substr(0, 2)) > 12 && std::stoi(r.getEnd().substr(0, 2)) <= 19)
        {
            for (int i = 0; i < 4; i++)
            {
                r.setReservate_time(r.date(), (r.getBefore() + "-" + r.getEnd()));
                r.setBefore(r.addTime(r.getBefore()));
                r.setEnd(r.addTime(r.getEnd()));
            }
        }
    }
    return 0;
}