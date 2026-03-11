#include "GetSchedule.h"
void Reservate::date()
{
    std::time_t t = std::time(nullptr);
    char buffer[11];
    std::tm local_tm = safeLocalTime(t);

    // 获取当前日期并格式化为字符串
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &local_tm);
    boost::gregorian::date today = boost::gregorian::from_simple_string(buffer);

    // 星期中文映射
    std::map<std::string, std::string> weekday_chinese = {
        {"Sunday", "星期日"},
        {"Monday", "星期一"},
        {"Tuesday", "星期二"},
        {"Wednesday", "星期三"},
        {"Thursday", "星期四"},
        {"Friday", "星期五"},
        {"Saturday", "星期六"}
    };

    // 获取未来7天的日期
    for (int i = 0; i < 7; i++)
    {
        // 计算第i天的日期
        boost::gregorian::date current_date = today + boost::gregorian::days(i);

        // 格式化日期字符串，确保月份和日期始终是两位数
        char date_buffer[11];
        snprintf(date_buffer, sizeof(date_buffer), "%d-%02d-%02d", 
                static_cast<int>(current_date.year()),
                static_cast<int>(current_date.month()),
                static_cast<int>(current_date.day()));
        
        std::string date_str(date_buffer);

        // 判断星期几
        boost::gregorian::greg_weekday day_of_week = current_date.day_of_week();
        std::string weekday_name = day_of_week.as_long_string();

        // 转换为中文
        std::string weekday_chinese_name = weekday_chinese[weekday_name];
        this->date_time.insert({date_str, weekday_chinese_name});
    }
};

nlohmann::json Reservate::slots(const std::string& start_time, const std::string& end_time)
{
    std::string current_start = start_time;
    std::string current_end = end_time;
    
    // 上午时段：09:00-12:00，每小时一个
    for (int i = 0; i < 3; ++i) {
        time_slots.push_back(current_start + "-" + current_end);
        current_start = addTime(current_start);
        current_end = addTime(current_end);
    }
    // 中午休息，跳过12:00-14:30
    current_start = "14:30";
    current_end = "15:30";

    // 下午时段：14:30-18:30，每小时一个
    for (int i = 0; i < 4; ++i) {
        time_slots.push_back(current_start + "-" + current_end);
        current_start = addTime(current_start);
        current_end = addTime(current_end);
    }

    return time_slots;
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

    // 生成未来7天的预约时间表
    for (auto &pair : r.date_time)
    {
        std::string date = pair.first;
        std::string weekday = pair.second;

        nlohmann::json day_schedule;
        day_schedule["year"] = std::stoi(date.substr(0, 4));    // 提取年份部分
        day_schedule["date"] = date.substr(5);                  // 去掉年份部分
        day_schedule["weekday"] = weekday;

        // 为每一天生成时间段
        day_schedule["time_slots"] = r.slots("09:00", "10:00");

        schedule.push_back(day_schedule);
    }

    return schedule;
}
