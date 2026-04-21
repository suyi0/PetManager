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

#include "../../../../models/user/User.h"

class Reservate
{
public:
    Reservate()
    {
        
    }

    void date();

    nlohmann::json slots(const std::string& start_time, const std::string& end_time);

    std::string addTime(const std::string &time);

    nlohmann::json generateSchedule();

    ~Reservate()
    {

    }

private:
    std::multimap<std::string, std::string> date_time;      // 七天日期时间表->( 日期 , 星期 )
    nlohmann::json schedule;                                // 最终时间表结果
};
