#ifndef __TIMEBASEDCONFIGMGR__
#define __TIMEBASEDCONFIGMGR__

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

class TimeBasedConfigMgr : public Orch
{
public:
    TimeBasedConfigMgr(std::vector<TableConnector>& connectors, DBConnector* appDb);
    using Orch::doTask;

private:
    DBConnector *m_appDb;
    TimeRangeConfigMap scheduledConfigurations;
    
    // Configuration functions
    bool validateConfiguration(const json &configJson);
    task_process_status enableTimeRange(const std::string &timeRangeName);
    task_process_status applyConfiguration(const std::string &configName, const json &configJson);
    bool applyTableConfiguration(const std::string &tableName, const json &tableKeyFields);

    
    // task processing functions
    task_process_status doProcessScheduledConfiguration(std::string timeRangeName, std::string configType, std::string configuration);
    task_process_status doProcessTimeRangeStatus(std::string timeRangeName, std::string status);
    
    
    // task functions
    void doTimeRangeTask(Consumer &consumer);
    void doScheduledConfigurationTask(Consumer &consumer);
    void doTask(Consumer &consumer);
};

}

#endif /* __TIMEBASEDCONFIGMGR__ */