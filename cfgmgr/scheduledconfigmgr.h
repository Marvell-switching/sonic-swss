#ifndef __SCHEDULEDCONFIGMGR_H__
#define __SCHEDULEDCONFIGMGR_H__

#include "dbconnector.h"
#include "producerstatetable.h"
#include "orch.h"
#include <unordered_map>
#include <string>
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;
// Define a type alias for the configuration data structure
using ConfigData = std::pair<std::string, json>; // Pair of configName and configuration JSON object
// using ConfigData = std::tuple<std::string, std::string, json>; // Tuple of configName, configType, and configuration JSON object
using ConfigList = std::vector<ConfigData>; // A list of configurations
using TimeRangeConfigMap = std::unordered_map<std::string, ConfigList>; // Maps time range names to lists of configurations

namespace swss {

class ScheduledConfigMgr : public Orch
{
public:
    ScheduledConfigMgr(std::vector<TableConnector>& connectors, DBConnector* appDb);
    using Orch::doTask;

private:
    DBConnector *m_appDb;
    TimeRangeConfigMap scheduledConfigurations;

    // Taken from https://www.codeproject.com/Tips/5299523/Regex-for-Cron-Expressions
    std::string m_cronMinuteRegex = R"((\*|(?:\*|(?:[0-9]|(?:[1-5][0-9])))\/(?:[0-9]|(?:[1-5][0-9]))|(?:[0-9]|(?:[1-5][0-9]))(?:(?:\-[0-9]|\-(?:[1-5][0-9]))?|(?:\,(?:[0-9]|(?:[1-5][0-9])))*)))";
    std::string m_cronHourRegex = R"((\*|(?:\*|(?:\*|(?:[0-9]|1[0-9]|2[0-3])))\/(?:[0-9]|1[0-9]|2[0-3])|(?:[0-9]|1[0-9]|2[0-3])(?:(?:\-(?:[0-9]|1[0-9]|2[0-3]))?|(?:\,(?:[0-9]|1[0-9]|2[0-3]))*)))";
    std::string m_cronDayOfMonthRegex = R"((\*|\?|L(?:W|\-(?:[1-9]|(?:[12][0-9])|3[01]))?|(?:[1-9]|(?:[12][0-9])|3[01])(?:W|\/(?:[1-9]|(?:[12][0-9])|3[01]))?|(?:[1-9]|(?:[12][0-9])|3[01])(?:(?:\-(?:[1-9]|(?:[12][0-9])|3[01]))?|(?:\,(?:[1-9]|(?:[12][0-9])|3[01]))*)))";
    std::string m_cronMonthRegex = R"((\*|(?:[1-9]|1[012]|JAN|FEB|MAR|APR|MAY|JUN|JUL|AUG|SEP|OCT|NOV|DEC)(?:(?:\-(?:[1-9]|1[012]|JAN|FEB|MAR|APR|MAY|JUN|JUL|AUG|SEP|OCT|NOV|DEC))?|(?:\,(?:[1-9]|1[012]|JAN|FEB|MAR|APR|MAY|JUN|JUL|AUG|SEP|OCT|NOV|DEC))*)))";
    std::string m_cronDayOfWeekRegex = R"((\*|\?|[0-6](?:L|\#[1-5])?|(?:[0-6]|SUN|MON|TUE|WED|THU|FRI|SAT)(?:(?:\-(?:[0-6]|SUN|MON|TUE|WED|THU|FRI|SAT))?|(?:\,(?:[0-6]|SUN|MON|TUE|WED|THU|FRI|SAT))*)))";

    
    // Configuration functions
    bool validateConfiguration(const json &configJson);

    task_process_status enableTimeRange(const std::string &timeRangeName);
    task_process_status applyConfiguration(const std::string &configName, const json &configJson);
    bool applyTableConfiguration(const std::string &tableName, const json &tableKeyFields);

    task_process_status disableTimeRange(const std::string &timeRangeName);
    task_process_status removeConfiguration(const std::string &configName, const json &configJson);
    bool removeTableConfiguration(const std::string &tableName);
    
    
    // task processing functions
    task_process_status doProcessScheduledConfiguration(std::string timeRangeName, std::string configType, std::string configuration);
    task_process_status doProcessTimeRangeStatus(std::string timeRangeName, std::string status);
    
    
    // task functions
    void doTimeRangeTask(Consumer &consumer);
    void doScheduledConfigurationTask(Consumer &consumer);
    void doTask(Consumer &consumer);
};

}

#endif /* __SCHEDULEDCONFIGMGR_H__ */