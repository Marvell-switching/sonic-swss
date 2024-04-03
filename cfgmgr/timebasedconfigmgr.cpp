#include <unistd.h>
#include <vector>
#include "select.h"
#include "schema.h"
#include <iostream>
#include <string>
#include "dbconnector.h"
#include "timebasedconfigmgr.h"
#include "json.h"
#include <nlohmann/json.hpp>
#include "tokenize.h"

using namespace std;
using namespace swss;
using json = nlohmann::json;

TimeBasedConfigMgr::TimeBasedConfigMgr(vector<TableConnector> &connectors, DBConnector *appDb) : Orch(connectors)
{
    m_appDb = appDb;
}

string join(const json &jsonArray, const string &delimiter)
{
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

bool TimeBasedConfigMgr::applyTableConfiguration(const std::string &tableName, const json &tableKeyFields)
{
    SWSS_LOG_ENTER();

    // Create a Table object for the given tableName
    Table tableObj(m_appDb, tableName);
    // Extract the key and fieldValues from the JSON object
    for (auto it = tableKeyFields.begin(); it != tableKeyFields.end(); ++it) {
        // Extract the key and value from the iterator
        const string& key = it.key();
        const json& fieldValuesJson = it.value();
        
        // Here, we convert the JSON tableDetails into the vector<FieldValueTuple> format.
        vector<FieldValueTuple> fieldValues = convertJsonToFieldValues(fieldValuesJson);
        if (fieldValues.empty())
        {
            SWSS_LOG_ERROR("Failed to convert JSON to FieldValueTuple for table: %s", tableName.c_str());
            return false;
        }

        // Create a Table object and set the field values
        tableObj.set(key, fieldValues);
    }
    return true;
}

task_process_status TimeBasedConfigMgr::applyConfiguration(const std::string &configName, const json &configJson)
{
    SWSS_LOG_ENTER();

    for (const auto &tableEntry : configJson.items())
    {
        string tableName = tableEntry.key();          // Table name
        const json &tableKeyFields = tableEntry.value(); // Table details

        if (!applyTableConfiguration(tableName, tableKeyFields))
        {
            SWSS_LOG_ERROR("Failed to apply configuration %s for table: %s", configName.c_str(), tableName.c_str());
            return task_process_status::task_failed;
        }
    }

    return task_process_status::task_success;
}

// TODO - Implement this function
bool TimeBasedConfigMgr::validateConfiguration(const json &configJson)
{
    SWSS_LOG_ENTER();
    return true;
}

task_process_status TimeBasedConfigMgr::doProcessScheduledConfiguration(string timeRangeName, string scheduledConfigName, string configuration)
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

        //TODO change to debug
        SWSS_LOG_INFO("===JSON CONFIGURATION STRING BEFORE PROCESS===");
        SWSS_LOG_INFO("%s", configuration.c_str());

        // Simple replacement of single quotes with double quotes
        // Necessary for json to succesfully parse the data
        replace(configuration.begin(), configuration.end(), '\'', '\"');

        SWSS_LOG_INFO("===JSON CONFIGURATION STRING AFTER PROCESS===");
        SWSS_LOG_INFO("%s", configuration.c_str());
        
        json configJson = json::parse(configuration);

        if (!validateConfiguration(configJson))
        {
            SWSS_LOG_ERROR("Configuration validation failed for %s", scheduledConfigName.c_str());
            return task_process_status::task_failed;
        }

        scheduledConfigurations[timeRangeName].emplace_back(scheduledConfigName, configJson);
        SWSS_LOG_INFO("Successfully added %s to time range %s ", scheduledConfigName.c_str(), timeRangeName.c_str());
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

task_process_status TimeBasedConfigMgr::doProcessTimeRangeStatus(string timeRangeName, string status)
{
    SWSS_LOG_ENTER();
    SWSS_LOG_INFO("Processing time range status for time range %s", timeRangeName.c_str());
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
            // Create the time range in the local db
            scheduledConfigurations[timeRangeName] = ConfigList{};
            SWSS_LOG_INFO("Time range %s created in local db, will retry to decide what to do next", timeRangeName.c_str());
            return task_process_status::task_need_retry;
        }

        // If the time range exists, apply the configuration based on the status
        if (status == "enabled")
            task_status = enableTimeRange(timeRangeName);
        else if (status == "disabled")
        {
            // Remove the configuration
            SWSS_LOG_INFO("Removing configuration for time range %s -- STUB", timeRangeName.c_str());
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

task_process_status TimeBasedConfigMgr::enableTimeRange(const string &timeRangeName)
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

    // Apply the configuration
    // scheduledConfigurations[timeRangeName].first is the configName
    // scheduledConfigurations[timeRangeName].second is the configuration JSON
    SWSS_LOG_INFO("Applying configuration for time range %s", timeRangeName.c_str());

    for (const auto &configData : scheduledConfigurations[timeRangeName])
    {
        configName = configData.first;
        configJson = configData.second;
        if (task_process_status::task_success != applyConfiguration(configName, configJson))
        {
            SWSS_LOG_ERROR("Could not apply configuration for time range %s, configName: %s", timeRangeName.c_str(), configName.c_str());
            return task_process_status::task_need_retry;
        }
        SWSS_LOG_INFO("Applied configuration for time range %s, configName: %s", timeRangeName.c_str(), configName.c_str());
    }
        return task_process_status::task_success;
}

void TimeBasedConfigMgr::doTimeRangeTask(Consumer &consumer)
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
                    // Can skip instead of returning invalid entry
                    task_status = task_process_status::task_invalid_entry;
                }
            }

            task_status = doProcessTimeRangeStatus(timeRangeName, status);
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

void TimeBasedConfigMgr::doScheduledConfigurationTask(Consumer &consumer)
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
                    // Can skip instead of returning invalid entry
                    task_status = task_process_status::task_invalid_entry;
                }
            }
            task_status = doProcessScheduledConfiguration(timeRangeName, scheduledConfigurationName, configuration);
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

void TimeBasedConfigMgr::doTask(Consumer &consumer)
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