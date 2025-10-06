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

    void start_reservate();

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

    nlohmann::json generateSchedule();

private:
    std::string name;
    std::string time_before;
    std::string time_end;
    std::multimap<std::string, std::string> reservate_time;
    std::multimap<std::string, std::string> date_time;
};