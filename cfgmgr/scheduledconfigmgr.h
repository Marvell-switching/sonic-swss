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
using ConfigStatus = std::unordered_map<std::string, bool>; // Maps configName to config status boolean
using ConfigData = std::unordered_map<std::string, json>; // Maps configName to JSON object
using TimeRangeConfigMap = std::unordered_map<std::string, ConfigData>; // Maps time range names to map of config data

namespace swss {

class ScheduledConfigMgr : public Orch
{
public:
    ScheduledConfigMgr(std::vector<TableConnector>& connectors, DBConnector* dynDb);
    using Orch::doTask;

private:
    DBConnector *m_dynDb;
    TimeRangeConfigMap scheduledConfigurations, unboundConfigurations;
    ConfigStatus scheduledConfigurationStatus;

    // Helper Functions
    DBConnector* getDBConnector(const std::string &tableName);
    std::string findTimeRangeByConfiguration(std::string scheduledConfigurationName);

    // Validation Functions
    bool validateConfiguration(const json &configJson);
    bool isTimeRangeActive(const std::string &timeRangeName);

    // Configuration Functions
    task_process_status enableTimeRange(const std::string &timeRangeName);
    task_process_status applyConfiguration(const std::string &configName, const json &configJson);
    bool applyTableConfiguration(const std::string &tableName, const json &tableKeyFields);

    task_process_status disableTimeRange(const std::string &timeRangeName);
    task_process_status removeConfiguration(const std::string &configName, const json &configJson);
    bool removeTableConfiguration(const std::string &tableName, const std::string &key);
    
    // Task Processing Functions
    task_process_status doProcessScheduledConfiguration(std::string timeRangeName, std::string configType, std::string configuration);
    task_process_status doProcessTimeRangeStatus(std::string timeRangeName, std::string status);
    
    // High-level Task Functions
    void doTimeRangeTask(Consumer &consumer);
    void doScheduledConfigurationTask(Consumer &consumer);
    void doTask(Consumer &consumer);
};

}

#endif /* __SCHEDULEDCONFIGMGR_H__ */