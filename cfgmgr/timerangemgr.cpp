#include <fstream>
#include <iostream>
#include <string.h>
#include <sstream>
#include "logger.h"
#include "dbconnector.h"
#include "timer.h"
#include "timerangemgr.h"
#include "tokenize.h"
#include <table.h>
#include "croncpp.h"

using namespace std;
using namespace swss;
using namespace cron;

TimeRangeMgr::TimeRangeMgr(DBConnector *cfgDb, DBConnector *stateDb, const vector<string> &tableNames) : Orch(cfgDb, tableNames),
                                                                                                         m_stateTimeRangeStatusTable(stateDb, STATE_TIME_RANGE_STATUS_TABLE_NAME)
{
}


bool is_time_in_range(const cronexpr& startExpr, const cronexpr& endExpr, const std::tm& currentTM) {
    std::time_t currentTime = mktime(const_cast<tm*>(&currentTM)); // Convert currentTM to time_t
    time_t previousMinute = currentTime - 60; // Go back one minute to ensure full coverage

    // Convert back to tm
    tm* prevTM = localtime(&previousMinute);

    time_t nextStartTime = cron_previous(startExpr, mktime(prevTM));
    time_t nextEndTime = cron_next(endExpr, mktime(prevTM));

    return (currentTime >= nextStartTime && currentTime < nextEndTime);
}

task_process_status TimeRangeMgr::writeCrontabFile(const string &fileName, const string &schedule, const string &command, bool deleteSelfAfterCompletion)
{
    string cronFileName = CRON_FILES_PATH_PREFIX_STR + fileName;
    ofstream crontabFile{cronFileName};

    if (crontabFile.fail())
    {
        SWSS_LOG_ERROR("Failed to create crontab file for %s", fileName.c_str());
        return task_process_status::task_need_retry;
    }
    crontabFile << schedule << " ";
    crontabFile << CRON_USERNAME_STR << " ";
    crontabFile << command;
    if (deleteSelfAfterCompletion)
    {
        crontabFile << " ; rm " << cronFileName;
    }
    crontabFile << endl;
    crontabFile.close();

    SWSS_LOG_DEBUG("Crontab file for %s has been created", fileName.c_str());
    return task_process_status::task_success;
}

// TODO add rollback mechanism
task_process_status TimeRangeMgr::createCronjobs(const string &taskName, const string &startTime, const string &endTime, bool runOnce)
{
    string enableCrontabName = taskName + "-enable";
    string disableCrontabName = taskName + "-disable";

    // Create command for enabling the task
    string command_enabled = string("/usr/bin/redis-cli -n ") + to_string(STATE_DB) + " HSET '" + STATE_TIME_RANGE_STATUS_TABLE_NAME + "|" + taskName + "' '" + TIME_RANGE_STATUS_STR + "' '" + TIME_RANGE_ENABLED_STR + "'";

    // Create command for disabling the task
    string command_disabled = string("/usr/bin/redis-cli -n ") + to_string(STATE_DB) + " HSET '" + STATE_TIME_RANGE_STATUS_TABLE_NAME + "|" + taskName + "' '" + TIME_RANGE_STATUS_STR + "' '" + TIME_RANGE_DISABLED_STR + "'";
    if (runOnce)
    {
        // Delete the time range configuration entry after the task has been disabled
        // writeCrontabFile() will delete the crontab file itself after the task has been executed
        command_disabled += " ; /usr/bin/redis-cli -n " + to_string(CONFIG_DB) + " del '" + CFG_TIME_RANGE_TABLE_NAME + "|" + taskName + "'";
    }

    // Service file for enabling the task
    if (writeCrontabFile(enableCrontabName, startTime, command_enabled, runOnce) != task_process_status::task_success)
    {
        return task_process_status::task_need_retry;
    }

    // Service file for disabling the task
    if (writeCrontabFile(disableCrontabName, endTime, command_disabled, runOnce) != task_process_status::task_success)
    {
        return task_process_status::task_need_retry;
    }

    SWSS_LOG_INFO("Succesfully created crontab files for %s", taskName.c_str());

    return task_process_status::task_success;
}
task_process_status TimeRangeMgr::doTimeRangeTaskDelete(const string &rangeName)
{
    SWSS_LOG_ENTER();
    string enableCrontabName = rangeName + "-enable";
    string disableCrontabName = rangeName + "-disable";

    // Delete the crontab files for the time range
    if (remove((CRON_FILES_PATH_PREFIX_STR + enableCrontabName).c_str()) != 0)
    {
        SWSS_LOG_ERROR("Failed to delete crontab file for %s", enableCrontabName.c_str());
        return task_process_status::task_need_retry;
    }
    if (remove((CRON_FILES_PATH_PREFIX_STR + disableCrontabName).c_str()) != 0)
    {
        SWSS_LOG_ERROR("Failed to delete crontab file for %s", disableCrontabName.c_str());
        return task_process_status::task_need_retry;
    }

    // Delete the time range status entry from the state db
    m_stateTimeRangeStatusTable.del(rangeName);

    return task_process_status::task_success;
}

task_process_status TimeRangeMgr::doTimeRangeTask(const string &rangeName, const vector<FieldValueTuple> &fieldValues)
{
    SWSS_LOG_ENTER();
    string start = "";
    string end = "";
    string runOnce = "";

    for (const auto &i : fieldValues)
    {
        if (fvField(i) == "start")
        {
            start = fvValue(i);
        }
        else if (fvField(i) == "end")
        {
            end = fvValue(i);
        }
        else if (fvField(i) == "runOnce")
        {
            runOnce = fvValue(i);
        }
        else
        {
            SWSS_LOG_ERROR("Time range %s has unknown field %s", rangeName.c_str(), fvField(i).c_str());
            return task_process_status::task_invalid_entry;
        }
    }

    if (start == "" || end == "")
    {
        SWSS_LOG_ERROR("Time range %s is missing start or end time", rangeName.c_str());
        return task_process_status::task_invalid_entry;
    }

    // Create cron files for time range and enable them
    // TODO sanitize inputs
    if (task_process_status::task_need_retry == createCronjobs(rangeName, start, end, (runOnce == "true")))
    {
        return task_process_status::task_need_retry;
    }

    // Check if time range should be active by default
    auto startExpr = make_cron(start);
    auto endExpr = make_cron(end);
    tm currentTM;
    string time_range_default_status = TIME_RANGE_DISABLED_STR;


    time_t currentTime = time(nullptr);
    localtime_r(&currentTime, &currentTM);


    if (is_time_in_range(startExpr, endExpr, currentTM))
    {
        SWSS_LOG_INFO("Time range %s is active", rangeName.c_str());
        time_range_default_status = TIME_RANGE_ENABLED_STR;
    }

    // Prepare state table field-values
    vector<FieldValueTuple> stateTableFieldValues;
    string key = rangeName;
    stateTableFieldValues.emplace_back(FieldValueTuple(TIME_RANGE_STATUS_STR, time_range_default_status));

    // Add time range status to range status table in state db
    m_stateTimeRangeStatusTable.set(key, stateTableFieldValues);

    return task_process_status::task_success;
}

void TimeRangeMgr::doTask(Consumer &consumer)
{
    SWSS_LOG_ENTER();

    string table_name = consumer.getTableName();

    auto it = consumer.m_toSync.begin();
    while (it != consumer.m_toSync.end())
    {
        KeyOpFieldsValuesTuple t = it->second;

        string keySeparator = CONFIGDB_KEY_SEPARATOR;
        vector<string> keys = tokenize(kfvKey(t), keySeparator[0]);
        string rangeName(keys[0]);

        string op = kfvOp(t);
        task_process_status task_status = task_process_status::task_success;
        if (op == SET_COMMAND)
        {
            if (table_name == CFG_TIME_RANGE_TABLE_NAME)
            {
                task_status = doTimeRangeTask(rangeName, kfvFieldsValues(t));
            }
        }
        else if (op == DEL_COMMAND)
        { 
            if (table_name == CFG_TIME_RANGE_TABLE_NAME)
            {
                task_status = doTimeRangeTaskDelete(rangeName);
            }
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
