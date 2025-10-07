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

nlohmann::json Reservate::slots(Reservate &r)
{
    // 这里可以根据需要实现时间段的生成逻辑
    if (std::stoi(r.getEnd().substr(0, 2)) <= 12)
    {
        for (int i = 0; i < 3; i++)
        {
            r.setTime_slots(r.getBefore() + "-" + r.getEnd());
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
            r.setTime_slots(r.getBefore() + "-" + r.getEnd());
            r.setBefore(r.addTime(r.getBefore()));
            r.setEnd(r.addTime(r.getEnd()));
        }
    }
    return r.time_slots;
}

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
    Reservate r;
    r.date(); // 生成日期
    nlohmann::json schedule;

    // 生成未来7天的预约时间表
    for (auto &pair : r.date_time)
    {
        std::string date = pair.first;
        std::string weekday = pair.second;

        nlohmann::json day_schedule;
        day_schedule["date"] = date;
        day_schedule["weekday"] = weekday;

        // 为每一天生成时间段
        day_schedule["time_slots"] = r.slots(r);

        schedule.push_back(day_schedule);
    }

    return schedule;
}