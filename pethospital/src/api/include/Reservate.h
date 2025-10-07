#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mysqlx/xdevapi.h>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <nlohmann/json.hpp>

#include "../include/User.h"

extern mysqlx::Session *g_db_session;
extern mysqlx::Schema *g_database;

class Reservate
{
public:
    Reservate();

    void date();

    nlohmann::json slots(Reservate &r);

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

    void setTime_slots(const std::string &slots)
    {
        this->time_slots.push_back( slots );
    }

    nlohmann::json generateSchedule();

private:
    std::string name;  // 医生姓名
    std::string time_before;  // 开始时间
    std::string time_end;  // 结束时间
    std::multimap<std::string, std::string> date_time;  // 七天日期时间表->( 日期 , 星期 )
    nlohmann::json time_slots = nlohmann::json::array();  // 预约时间段
};