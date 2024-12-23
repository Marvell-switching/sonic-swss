#include <unistd.h>
#include <vector>
#include "select.h"
#include "schema.h"
#include <iostream>
#include <string>
#include "dbconnector.h"
#include "scheduledconfigmgr.h"
#include "json.h"
#include <nlohmann/json.hpp>
#include "tokenize.h"

using namespace std;
using namespace swss;
using json = nlohmann::json;

ScheduledConfigMgr::ScheduledConfigMgr(vector<TableConnector> &connectors, DBConnector *appDb) : Orch(connectors)
{
    m_appDb = appDb;
}

string ScheduledConfigMgr::findTimeRangeByConfiguration(string scheduledConfigurationName) {
    for (const auto& pair : scheduledConfigurations) {
        const std::string& timeRangeName = pair.first;
        const ConfigData& configDataMap = pair.second;
        
        // Check if the scheduledConfigurationName exists in the inner ConfigData map
        if (configDataMap.find(scheduledConfigurationName) != configDataMap.end()) {
            return timeRangeName;
        }
    }
    return ""; // Return an empty string if not found
}

DBConnector* ScheduledConfigMgr::getDBConnector(const string &tableName){

    Consumer* tableConsumer = static_cast<Consumer *>(getExecutor(tableName));
    if (!tableConsumer)
    {
        SWSS_LOG_ERROR("Failed to get consumer for %s", tableName.c_str());
        return nullptr;
    }
    DBConnector* tableDBConnector = const_cast<DBConnector*>(tableConsumer->getDbConnector());
    if (!tableDBConnector) {
        SWSS_LOG_ERROR("Failed to get DB connector for %s", tableName.c_str());
        return nullptr;
    }
    return tableDBConnector;
}

string join(const json &jsonArray, const string &delimiter)
{
    // Join the elements of the JSON array into a single string
    string result;

    for (auto it = jsonArray.begin(); it != jsonArray.end(); ++it)
    {
        if (!result.empty())
        {
            result += delimiter;
        }
        // Convert each element to a string, handling non-string types appropriately
        if (it->is_string())
        {
            result += it->get<string>();
        }
        else
        {
            // For non-string types, use dump() to serialize
            // dump() serializes the json object to a string representation
            // Since dump() includes quotes around strings, we remove them for consistency
            string element = it->dump();
            // Optionally trim quotes if desired, especially if consistency with direct strings is needed
            if (element.front() == '"' && element.back() == '"')
            {
                element = element.substr(1, element.length() - 2);
            }
            result += element;
        }
    }

    return result;
}

vector<FieldValueTuple> convertJsonToFieldValues(const json &jsonObj)
{
    vector<FieldValueTuple> fieldValues{};
    for (const auto &item : jsonObj.items())
    {
        string key = item.key();
        if (item.value().is_primitive())
        {
            // Removing quotes from primitive types to match the expected format
            string value = item.value().dump();
            if (value.front() == '"' && value.back() == '"')
            {
                value = value.substr(1, value.length() - 2);
            }
            fieldValues.emplace_back(key, value);
        }
        else if (item.value().is_array())
        {
            string arrayValues = join(item.value(), ",");
            fieldValues.emplace_back(key, arrayValues);
        }
    }
    return fieldValues;
}

bool ScheduledConfigMgr::isTimeRangeActive(const string &timeRangeName)
{
    SWSS_LOG_ENTER();
    shared_ptr<string> statusPtr{};
    string key = "";
    DBConnector* timeRangeStatusDBConnector = getDBConnector(STATE_TIME_RANGE_STATUS_TABLE_NAME);
    if (!timeRangeStatusDBConnector)
        return false;

    key = STATE_TIME_RANGE_STATUS_TABLE_NAME +  SonicDBConfig::getSeparator(timeRangeStatusDBConnector) + timeRangeName;
    statusPtr = timeRangeStatusDBConnector->hget(key, "status");
    if (!statusPtr){
        SWSS_LOG_ERROR("Failed to get time range status for %s", timeRangeName.c_str());
        return false;
    }
    
    return *statusPtr=="active";
}

bool ScheduledConfigMgr::applyTableConfiguration(const std::string &tableName, const json &tableKeyFields)
{
    SWSS_LOG_ENTER();

    // Create a Table object for the given tableName
    ProducerStateTable tableObj(m_appDb, tableName);
    
    // Extract the key and fieldValues from the JSON object
    for (auto it = tableKeyFields.begin(); it != tableKeyFields.end(); ++it) {

        // Extract the key and value from the iterator
        const string& key = it.key();
        const json& fieldValuesJson = it.value();

        SWSS_LOG_DEBUG("Extracted key %s from table %s", key.c_str(), tableName.c_str());
        
        // Here, we convert the JSON tableDetails into the vector<FieldValueTuple> format.
        vector<FieldValueTuple> fieldValues = convertJsonToFieldValues(fieldValuesJson);
        if (fieldValues.empty())
        {
            SWSS_LOG_ERROR("Failed to convert JSON to FieldValueTuple for table: %s", tableName.c_str());
            return false;
        }

        SWSS_LOG_DEBUG("Setting key %s in table %s", key.c_str(), tableName.c_str());
        // Create a Table object and set the field values
        tableObj.set(key, fieldValues);
    }
    return true;
}

bool ScheduledConfigMgr::removeTableConfiguration(const string &tableName, const string &key)
{
    SWSS_LOG_ENTER();

    // Create a Table object for the given tableName
    ProducerStateTable tableObj(m_appDb, tableName);

    // Create a Table object and set the field values
    tableObj.del(key);
    return true;
}

task_process_status ScheduledConfigMgr::applyConfiguration(const string &configName, const json &configJson)
{
    SWSS_LOG_ENTER();
    for (const auto &tableEntry : configJson.items())
    {
        string tableName = tableEntry.key();             // Table name
        const json &tableKeyFields = tableEntry.value(); // Table details

        SWSS_LOG_DEBUG("Found table name %s inside configuration %s", tableName.c_str(), configName.c_str());
        if (!applyTableConfiguration(tableName, tableKeyFields))
        {
            SWSS_LOG_ERROR("Failed to apply configuration %s for table: %s", configName.c_str(), tableName.c_str());
            return task_process_status::task_failed;
        }
    }

    return task_process_status::task_success;
}

task_process_status ScheduledConfigMgr::removeConfiguration(const string &configName, const json &configJson)
{
    SWSS_LOG_ENTER();
    std::string tableName;
    std::string key;

    for (const auto &tableEntry : configJson.items()) {
        tableName = tableEntry.key(); // e.g., "ACL_TABLE_TABLE" or "ACL_TABLE_TABLE:ACL_TABLE_NAME"
        const json &innerObject = tableEntry.value();

        // Check if the outer key already contains the entire table name and key
        size_t pos = tableName.find(':');
        if (pos != std::string::npos) {
            key = tableName.substr(pos + 1); // Extract the key part after ':'
            tableName = tableName.substr(0, pos); // Extract the table name part before ':'
        } else if (innerObject.is_object()) {
            // Iterate through the inner object to get the key
            for (const auto &innerEntry : innerObject.items()) {
                key = innerEntry.key(); // e.g., "ACL_TABLE_NAME"
                break; // We only need the first key for this function
            }
        } else {
            SWSS_LOG_ERROR("Expected JSON object for key: %s", tableName.c_str());
            return task_process_status::task_failed;
        }

        // Call removeTableConfiguration with the parsed tableName and key
        if (!removeTableConfiguration(tableName, key)) {
            SWSS_LOG_ERROR("Failed to remove configuration %s for table: %s with key: %s", configName.c_str(), tableName.c_str(), key.c_str());
            return task_process_status::task_failed;
        }
    }

    return task_process_status::task_success;
}

// TODO - Implement this function
bool ScheduledConfigMgr::validateConfiguration(const json &configJson)
{
    SWSS_LOG_ENTER();
    return true;
}

task_process_status ScheduledConfigMgr::doProcessScheduledConfiguration(string timeRangeName, string scheduledConfigName, string configuration)
{
    SWSS_LOG_ENTER();
    SWSS_LOG_INFO("Processing scheduled configuration %s for time range %s", scheduledConfigName.c_str(), timeRangeName.c_str());
    task_process_status task_status = task_process_status::task_success;

    // Validate timeRangeName, scheduledConfigName, and configuration
    if (timeRangeName.empty() || scheduledConfigName.empty() || configuration.empty())
    {
        SWSS_LOG_ERROR("Invalid arguments for scheduled configuration: timeRangeName, scheduledConfigName, or configuration is empty");
        return task_process_status::task_invalid_entry;
    }

    try
    {
        // Parse the configuration string into a JSON object for validation
        // Assuming the configuration is in a JSON string format
        SWSS_LOG_DEBUG("===JSON CONFIGURATION STRING BEFORE PROCESS===");
        SWSS_LOG_DEBUG("%s", configuration.c_str());

        // Simple replacement of single quotes with double quotes
        // Necessary for json to succesfully parse the data
        replace(configuration.begin(), configuration.end(), '\'', '\"');

        SWSS_LOG_DEBUG("===JSON CONFIGURATION STRING AFTER PROCESS===");
        SWSS_LOG_DEBUG("%s", configuration.c_str());
        
        json configJson = json::parse(configuration);

        if (!validateConfiguration(configJson))
        {
            SWSS_LOG_ERROR("Configuration validation failed for %s", scheduledConfigName.c_str());
            return task_process_status::task_failed;
        }

        // Verify time range does not exist in the scheduledConfigurations hashmap        
        if (scheduledConfigurations[timeRangeName].find(scheduledConfigName) != scheduledConfigurations[timeRangeName].end())
        {
            SWSS_LOG_ERROR("Scheduled configuration %s already exists for time range %s", scheduledConfigName.c_str(), timeRangeName.c_str());
            return task_process_status::task_failed;
        }

        // Add the configuration to the scheduledConfigurations hashmap
        scheduledConfigurations[timeRangeName][scheduledConfigName] =  configJson;
        SWSS_LOG_INFO("Successfully added %s to time range %s ", scheduledConfigName.c_str(), timeRangeName.c_str());

        // Apply the configuration if the time range currently is active
        if (isTimeRangeActive(timeRangeName))
        {
            if (task_process_status::task_success != applyConfiguration(scheduledConfigName, configJson))
            {
                SWSS_LOG_ERROR("Could not apply configuration for time range %s, configName: %s", timeRangeName.c_str(), scheduledConfigName.c_str());
                return task_process_status::task_need_retry;
            }
            // Add the configuration to the scheduledConfigurationStatus hashmap with status true
            scheduledConfigurationStatus[scheduledConfigName] = true;
            SWSS_LOG_INFO("Applied configuration for time range %s, configName: %s", timeRangeName.c_str(), scheduledConfigName.c_str());
        } else {
            // Add the configuration to the scheduledConfigurationStatus hashmap with status false
            scheduledConfigurationStatus[scheduledConfigName] = false;
        }
    }
    catch (const json::exception &e)
    {
        SWSS_LOG_ERROR("JSON parsing error: %s", e.what());
        task_status = task_process_status::task_failed;
    }
    catch (const std::exception &e)
    {
        SWSS_LOG_ERROR("Error processing scheduled configuration: %s", e.what());
        task_status = task_process_status::task_failed;
    }
    return task_status;
}

task_process_status ScheduledConfigMgr::doProcessTimeRangeStatus(string timeRangeName, string status)
{
    SWSS_LOG_ENTER();
    task_process_status task_status = task_process_status::task_success;

    // Validate timeRangeName and status
    if (timeRangeName.empty() || status.empty())
    {
        SWSS_LOG_ERROR("Invalid arguments for time range status: timeRangeName or status is empty");
        return task_process_status::task_invalid_entry;
    }

    try
    {
        // Check if the time range exists in the scheduledConfigurations hashmap
        if (scheduledConfigurations.find(timeRangeName) == scheduledConfigurations.end())
        {
            SWSS_LOG_INFO("Time range %s is being created in the local db", timeRangeName.c_str());
            // Create the time range in the local db with default value
            scheduledConfigurations[timeRangeName];

            SWSS_LOG_INFO("Adding unbound configurations for time range %s if exist", timeRangeName.c_str());
            if (unboundConfigurations.find(timeRangeName) != unboundConfigurations.end())
            {
                for (const auto &configData : unboundConfigurations[timeRangeName])
                {
                    SWSS_LOG_NOTICE("Binding configuration %s to time range %s", configData.first.c_str(), timeRangeName.c_str());
                    scheduledConfigurations[timeRangeName].insert(configData);
                }
                unboundConfigurations.erase(timeRangeName);
            }
            SWSS_LOG_INFO("Finished adding unbound configurations for time range %s if exist", timeRangeName.c_str());
            SWSS_LOG_INFO("Time range %s created in local db, will retry to decide what to do next", timeRangeName.c_str());
            return task_process_status::task_need_retry;
        }

        // If the time range exists, apply the configuration based on the status
        SWSS_LOG_INFO("Time range %s is %s", timeRangeName.c_str(), status.c_str());
        if (status == "active"){
            task_status = enableTimeRange(timeRangeName);
        }
        else if (status == "inactive")
        {
            task_status = disableTimeRange(timeRangeName);
        }
        else
        {
            SWSS_LOG_ERROR("Invalid status for time range %s: %s", timeRangeName.c_str(), status.c_str());
            task_status = task_process_status::task_failed;
        }
    }
    catch (const json::exception &e)
    {
        SWSS_LOG_ERROR("JSON parsing error: %s", e.what());
        task_status = task_process_status::task_failed;
    }
    catch (const std::exception &e)
    {
        SWSS_LOG_ERROR("Error processing time range status: %s", e.what());
        task_status = task_process_status::task_failed;
    }

    return task_status;
}

task_process_status ScheduledConfigMgr::enableTimeRange(const string &timeRangeName)
{
    SWSS_LOG_ENTER();

    string configName{};
    json configJson{};

    // Check if there are any configurations for the time range
    if (scheduledConfigurations[timeRangeName].empty())
    {
        SWSS_LOG_INFO("No configuration found for time range %s", timeRangeName.c_str());
        return task_process_status::task_success;
    }
    SWSS_LOG_DEBUG("Found configuration for time range %s", timeRangeName.c_str());

    // Apply the configuration
    // scheduledConfigurations[timeRangeName].first is the configName
    // scheduledConfigurations[timeRangeName].second is the configuration JSON
    SWSS_LOG_INFO("Applying configurations for time range %s", timeRangeName.c_str());

    for (const auto &configData : scheduledConfigurations[timeRangeName])
    {
        configName = configData.first;
        configJson = configData.second;
        SWSS_LOG_INFO("Applying configuration %s for time range %s", configName.c_str(), timeRangeName.c_str());
        if (task_process_status::task_success != applyConfiguration(configName, configJson))
        {
            SWSS_LOG_ERROR("Could not apply configuration for time range %s, configName: %s", timeRangeName.c_str(), configName.c_str());
            return task_process_status::task_need_retry;
        }
        scheduledConfigurationStatus[configName] = true;
        SWSS_LOG_INFO("Applied configuration for time range %s, configName: %s", timeRangeName.c_str(), configName.c_str());
    }
        return task_process_status::task_success;
}

task_process_status ScheduledConfigMgr::disableTimeRange(const string &timeRangeName)
{
    SWSS_LOG_ENTER();

    string configName{};
    json configJson{};

    // Check if there are any configurations for the time range
    if (scheduledConfigurations[timeRangeName].empty())
    {
        SWSS_LOG_INFO("No configuration found for time range %s", timeRangeName.c_str());
        return task_process_status::task_success;
    }

    // Remove the configuration
    // scheduledConfigurations[timeRangeName].first is the configName
    // scheduledConfigurations[timeRangeName].second is the configuration JSON
    SWSS_LOG_INFO("Removing configurations for time range %s", timeRangeName.c_str());

    for (const auto &configData : scheduledConfigurations[timeRangeName])
    {
        configName = configData.first;
        configJson = configData.second;
        if (task_process_status::task_success != removeConfiguration(configName, configJson))
        {
            SWSS_LOG_ERROR("Could not remove configuration for time range %s, configName: %s", timeRangeName.c_str(), configName.c_str());
            return task_process_status::task_need_retry;
        }
        scheduledConfigurationStatus[configName] = false;
        SWSS_LOG_INFO("Removed configuration for time range %s, configName: %s", timeRangeName.c_str(), configName.c_str());
    }
        return task_process_status::task_success;
}

void ScheduledConfigMgr::doTimeRangeTask(Consumer &consumer)
{
    SWSS_LOG_ENTER();
    auto it = consumer.m_toSync.begin();
    while (it != consumer.m_toSync.end())
    {
        string timeRangeName = "";
        string status = "";

        KeyOpFieldsValuesTuple t = it->second;

        string keySeparator = CONFIGDB_KEY_SEPARATOR;
        vector<string> keys = tokenize(kfvKey(t), keySeparator[0]);
        timeRangeName = keys[0];
        string op = kfvOp(t);

        SWSS_LOG_INFO("OP: %s, TABLE_ID: %s", op.c_str(), timeRangeName.c_str());
        task_process_status task_status = task_process_status::task_success;
        if (op == SET_COMMAND)
        {
            for (const auto &i : kfvFieldsValues(t))
            {
                if (fvField(i) == "status")
                {
                    status = fvValue(i);
                }
                else
                {
                    SWSS_LOG_ERROR("%s has unknown field %s", STATE_TIME_RANGE_STATUS_TABLE_NAME, fvField(i).c_str());
                    task_status = task_process_status::task_invalid_entry;
                    break;
                }
            }

            if (task_status == task_process_status::task_success)
            {
                SWSS_LOG_INFO("Processing time range %s with status %s", timeRangeName.c_str(), status.c_str());
                task_status = doProcessTimeRangeStatus(timeRangeName, status);
            }
        } else if (op == DEL_COMMAND)
        {
            // Disable, and then remove the time range
            if (scheduledConfigurations.find(timeRangeName) != scheduledConfigurations.end())
            {
                if (task_process_status::task_success != disableTimeRange(timeRangeName))
                {
                    SWSS_LOG_ERROR("Could not disable time range %s", timeRangeName.c_str());
                    task_status = task_process_status::task_need_retry;
                }
                SWSS_LOG_INFO("Disabled time range %s", timeRangeName.c_str());
            }
            // Save configurations for future creation of time range
            unboundConfigurations[timeRangeName] = scheduledConfigurations[timeRangeName];

            // Remove time range
            scheduledConfigurations.erase(timeRangeName);
        }
        switch (task_status)
        {
        case task_process_status::task_failed:
            SWSS_LOG_ERROR("Failed to process table update");
            return;
        case task_process_status::task_need_retry:
            SWSS_LOG_INFO("Unable to process table update. Will retry...");
            ++it;
            break;
        case task_process_status::task_invalid_entry:
            SWSS_LOG_ERROR("Failed to process invalid entry, drop it");
            it = consumer.m_toSync.erase(it);
            break;
        default:
            it = consumer.m_toSync.erase(it);
            break;
        }
    }
}

void ScheduledConfigMgr::doScheduledConfigurationTask(Consumer &consumer)
{
    SWSS_LOG_ENTER();
    auto it = consumer.m_toSync.begin();
    while (it != consumer.m_toSync.end())
    {
        string timeRangeName = "";
        string configType = "";
        string configuration = "";

        KeyOpFieldsValuesTuple t = it->second;

        string keySeparator = CONFIGDB_KEY_SEPARATOR;
        vector<string> keys = tokenize(kfvKey(t), keySeparator[0]);
        string scheduledConfigurationName(keys[0]);

        string op = kfvOp(t);
        task_process_status task_status = task_process_status::task_success;
        if (op == SET_COMMAND)
        {
            for (const auto &i : kfvFieldsValues(t))
            {
                if (fvField(i) == "time_range")
                {
                    timeRangeName = fvValue(i);
                }
                else if (fvField(i) == "config_type")
                {
                    configType = fvValue(i);
                }
                else if (fvField(i) == "configuration")
                {
                    configuration = fvValue(i);
                }
                else
                {
                    SWSS_LOG_ERROR("%s has unknown field %s", CFG_SCHEDULED_CONFIGURATION_TABLE_NAME, fvField(i).c_str());
                    task_status = task_process_status::task_invalid_entry;
                }
            }
            if (task_status == task_process_status::task_success)
            {
                task_status = doProcessScheduledConfiguration(timeRangeName, scheduledConfigurationName, configuration);
            }
        } else if (op == DEL_COMMAND)
        {
            if (scheduledConfigurationStatus.find(scheduledConfigurationName) != scheduledConfigurationStatus.end()){
                if (scheduledConfigurationStatus[scheduledConfigurationName]){
                    // Get scheduled configuration time range name
                    timeRangeName = findTimeRangeByConfiguration(scheduledConfigurationName);

                    // Remove the configuration
                    if (scheduledConfigurations.find(timeRangeName) != scheduledConfigurations.end())
                    {
                        if (task_process_status::task_success != removeConfiguration(scheduledConfigurationName, scheduledConfigurations[timeRangeName][scheduledConfigurationName]))
                        {
                            SWSS_LOG_ERROR("Could not remove configuration for time range %s, configName: %s", timeRangeName.c_str(), scheduledConfigurationName.c_str());
                            task_status = task_process_status::task_need_retry;
                        }
                        scheduledConfigurationStatus.erase(scheduledConfigurationName);
                        scheduledConfigurations[timeRangeName].erase(scheduledConfigurationName);
                        SWSS_LOG_INFO("Removed configuration for time range %s, configName: %s", timeRangeName.c_str(), scheduledConfigurationName.c_str());
                    }
                }
            } else {
                SWSS_LOG_ERROR("Scheduled configuration %s does not exist", scheduledConfigurationName.c_str());
                task_status = task_process_status::task_failed;
            }
        }

        switch (task_status)
        {
        case task_process_status::task_failed:
            SWSS_LOG_ERROR("Failed to process table update");
            it = consumer.m_toSync.erase(it);
            return;
        case task_process_status::task_need_retry:
            SWSS_LOG_INFO("Unable to process table update. Will retry...");
            ++it;
            break;
        case task_process_status::task_invalid_entry:
            SWSS_LOG_ERROR("Failed to process invalid entry, drop it");
            it = consumer.m_toSync.erase(it);
            break;
        default:
            it = consumer.m_toSync.erase(it);
            break;
        }
    }
}

void ScheduledConfigMgr::doTask(Consumer &consumer)
{
    SWSS_LOG_ENTER();

    string table_name = consumer.getTableName();
    if (table_name == CFG_SCHEDULED_CONFIGURATION_TABLE_NAME)
    {
        doScheduledConfigurationTask(consumer);
    }
    else if (table_name == STATE_TIME_RANGE_STATUS_TABLE_NAME)
    {
        doTimeRangeTask(consumer);
    }
}