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
    Reservate()
    {
        
    }

    void date();

    nlohmann::json slots(const std::string& start_time, const std::string& end_time);

    std::string addTime(const std::string &time);

    void setTime_slots(const std::string &slots)
    {
        this->time_slots.push_back( slots );
    }

    nlohmann::json generateSchedule();

private:
    std::string name;  // 医生姓名
    std::multimap<std::string, std::string> date_time;  // 七天日期时间表->( 日期 , 星期 )
    nlohmann::json time_slots = nlohmann::json::array();  // 预约时间段
};