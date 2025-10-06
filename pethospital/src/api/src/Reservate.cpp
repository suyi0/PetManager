#include "../include/Reservate.h"
Reservate::Reservate()
{
    this->time_before = "09:00";
    this->time_end = "10:00";
}
void Reservate::date()
{
    std::time_t t = std::time(nullptr);
    char buffer[11];
    for (int i = 0; i < 7; i++)
    {
        // 获取当前日期并格式化为字符串
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&t));
        boost::gregorian::date today = boost::gregorian::from_simple_string(buffer);
        // 判断星期几
        boost::gregorian::greg_weekday day_of_week = today.day_of_week();
        std::string weekday_name = day_of_week.as_long_string();

        this->date_time.insert({buffer, weekday_name});
    }
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


nlohmann::json Reservate::generateSchedule()
{
    nlohmann::json schedule;
    
    // 生成未来7天的预约时间表
    for (auto &pair : this->date_time) {
        std::string date = pair.first;
        std::string weekday = pair.second;
        
        nlohmann::json day_schedule;
        day_schedule["date"] = date;
        day_schedule["weekday"] = weekday;
        
        // 为每一天生成时间段 (示例：9:00-17:00，每小时一个时段)
        nlohmann::json time_slots = nlohmann::json::array();
        std::string start_time = "09:00";
        std::string end_time = "17:00";
        
        std::string current_start = start_time;
        while (current_start < end_time) {
            std::string current_end = addTime(current_start);
            if (current_end <= end_time) {
                time_slots.push_back(current_start + "-" + current_end);
            }
            current_start = current_end;
        }
        
        day_schedule["time_slots"] = time_slots;
        schedule.push_back(day_schedule);
    }
    
    return schedule;
}
void Reservate::start_reservate()
{
    // 获取医生数据
    User u;
    try
    {
        mysqlx::Table doctor_table = g_database->getTable("doctors");
        mysqlx::RowResult result;
        result = doctor_table.select("id", "name", "password", "phone", "email", "CAST(birthday AS CHAR)", "creation_time", "address_id", "head_image").execute();

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
    }
    if (u.token == true)
    {
        Reservate r;
        r.date();
        std::string first_date = "";
        for (auto &pair : r.date_time)
        {
            first_date = pair.first;
            if (std::stoi(r.getEnd().substr(0, 2)) <= 12)
            {
                for (int i = 0; i < 3; i++)
                {
                    r.setReservate_time(first_date, (r.getBefore() + "-" + r.getEnd()));
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
                    r.setReservate_time(first_date, (r.getBefore() + "-" + r.getEnd()));
                    r.setBefore(r.addTime(r.getBefore()));
                    r.setEnd(r.addTime(r.getEnd()));
                }
            }
        }
    }
}