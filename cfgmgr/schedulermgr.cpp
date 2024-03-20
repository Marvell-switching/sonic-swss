#include <fstream>
#include <iostream>
#include <string.h>
#include "logger.h"
#include "dbconnector.h"
#include "producerstatetable.h"
#include "tokenize.h"
#include "ipprefix.h"
#include "timer.h"
#include "schedulermgr.h"
#include "exec.h"
#include "shellcmd.h"
#include "warm_restart.h"
#include "converter.h"

using namespace std;
using namespace swss;

#define PORT_NAME_GLOBAL "global"

SchedulerMgr::SchedulerMgr(DBConnector *cfgDb, DBConnector *stateDb, const vector<string> &tableNames):
        Orch(cfgDb, tableNames),
        m_cfgTimeRangeTable(cfgDb, CFG_TIME_RANGE_TABLE_NAME),
        m_cfgScheduledConfigurationTable(cfgDb, CFG_SCHEDULED_CONFIGURATION_TABLE_NAME),
        m_stateTimeRangeStatusTable(stateDb, STATE_TIME_RANGE_STATUS_TABLE_NAME)
{}

bool SchedulerMgr::reloadSystemd() {
    // Reload systemd to recognize changes
    return system(SYSTEMD_RELOAD_CMD_STR) == 0;
}

bool SchedulerMgr::enableSystemdTimer(const std::string& timerFileName) {
    // Enable the systemd timer
    string enableTimerCmd = SYSTEMD_TIMER_ENABLE_PREFIX_CMD_STR + timerFileName;
    return system(enableTimerCmd.c_str()) == 0;
}

task_process_status SchedulerMgr::writeSystemdUnitFile(const string& fileName, const string& taskType, const string& taskCommand) {
    string unitFilePath = SYSTEMD_FILES_PATH_PREFIX_STR + fileName;
    ofstream unitFile{unitFilePath};
    if (unitFile.fail()) {
        SWSS_LOG_ERROR("Failed to create systemd file for %s", fileName.c_str());
        return task_process_status::task_need_retry;
    }
    unitFile << "[Unit]\n";
    unitFile << "Description=" << fileName << " service\n\n";
    unitFile << "[Service]\n";
    unitFile << "Type=" << taskType << "\n";
    unitFile << "ExecStart=" << taskCommand << "\n";
    unitFile.close();

    SWSS_LOG_DEBUG("Systemd unit file for %s has been created", fileName.c_str());
    return task_process_status::task_success;
}

task_process_status SchedulerMgr::writeSystemdTimerFile(const string& fileName, const string& taskOnCalendar, bool isPersistent) {
    string timerFilePath = SYSTEMD_FILES_PATH_PREFIX_STR + fileName;
    ofstream timerFile{timerFilePath};
    if (timerFile.fail()) {
        SWSS_LOG_ERROR("Failed to create systemd timer file for %s", fileName.c_str());
        return task_process_status::task_need_retry;
    }
    timerFile << "[Unit]\n";
    timerFile << "Description=Timer for " << fileName << " service\n\n";
    timerFile << "[Timer]\n";
    timerFile << "OnCalendar=" << taskOnCalendar << "\n";
    if (isPersistent) {
        timerFile << "Persistent=true\n";
    }
    timerFile << "\n";
    timerFile << "[Install]\n";
    timerFile << "WantedBy=timers.target\n";
    timerFile.close();

    SWSS_LOG_DEBUG("Systemd timer file for %s has been created", fileName.c_str());
    return task_process_status::task_success;
}

// TODO add rollback mechanism
task_process_status SchedulerMgr::createSystemdFiles(const string& taskName, const string& startTime, const string& endTime) {
    // Service file for enabling the task
    // TODO Replace with actual command 
    string enableServiceFileName = taskName + "-enable" + SYSTEMD_UNIT_FILE_EXTENSION_STR;
    if (writeSystemdUnitFile(enableServiceFileName, "oneshot", "/bin/echo 'Starting " + taskName + "'") != task_process_status::task_success) {
        return task_process_status::task_need_retry;
    }

    // Timer file for enabling the task
    string enableTimerFileName = taskName + "-enable" + SYSTEMD_TIMER_FILE_EXTENSION_STR;
    if (writeSystemdTimerFile(enableTimerFileName, startTime, false) != task_process_status::task_success) {
        return task_process_status::task_need_retry;
    }

    // Service file for disabling the task
    // TODO Replace with actual command 
    string disableServiceFileName = taskName + "-disable" + SYSTEMD_UNIT_FILE_EXTENSION_STR;
    if (writeSystemdUnitFile(disableServiceFileName, "oneshot", "/bin/echo 'Stopping " + taskName + "'") != task_process_status::task_success) {
        return task_process_status::task_need_retry;
    }

    // Timer file for disabling the task
    string disableTimerFileName = taskName + "-disable" + SYSTEMD_TIMER_FILE_EXTENSION_STR;
    if (writeSystemdTimerFile(disableTimerFileName, endTime, false) != task_process_status::task_success) {
        return task_process_status::task_need_retry;
    } 

    SWSS_LOG_INFO("Succesfully created Systemd files for %s", taskName.c_str());

    // Reload systemd to pick up changes, then enable systemd timer files
    if (!reloadSystemd()) {
        SWSS_LOG_ERROR("Failed to reload systemd");
        return task_process_status::task_need_retry;
    }
    if (!enableSystemdTimer(enableTimerFileName) || !enableSystemdTimer(disableTimerFileName)) {
        SWSS_LOG_ERROR("Failed to enable systemd timer files for %s", taskName.c_str());
        return task_process_status::task_need_retry;
    }

    SWSS_LOG_DEBUG("Succesfully reloaded and enabled systemd timer files for %s", taskName.c_str());
    return task_process_status::task_success;
}

task_process_status SchedulerMgr::doTimeRangeTask(const string& rangeName, const vector<FieldValueTuple> &fieldValues){
    SWSS_LOG_ENTER();
    string start = "";
    string end = "";

    // Set time range status to disabled by default
    vector<FieldValueTuple> stateTableFieldValues;
    string key = rangeName;
    stateTableFieldValues.emplace_back(FieldValueTuple(TIME_RANGE_STATUS_STR, TIME_RANGE_DISABLED_STR));

    for (const auto &i : fieldValues)
    {
        if (fvField(i) == "start"){
            start = fvValue(i);
        } else if (fvField(i) == "end"){
            end = fvValue(i);
        } else {
            SWSS_LOG_ERROR("Time range %s has unknown field %s", rangeName.c_str(), fvField(i).c_str());
            // Can skip instead of returning invalid entry
            return task_process_status::task_invalid_entry;
        }
    }

    if (start == "" || end == ""){
        SWSS_LOG_ERROR("Time range %s is missing start or end time", rangeName.c_str());
        return task_process_status::task_invalid_entry;
    }

    // Create systemd files for time range and enable them
    // TODO sanitize inputs
    if (task_process_status::task_need_retry == createSystemdFiles(rangeName, start, end)) {
        return task_process_status::task_need_retry;
    }
    
    // Add time range status to range status table in state db
    m_stateTimeRangeStatusTable.set(key, stateTableFieldValues);

    return task_process_status::task_success;
}

void SchedulerMgr::doTask(Consumer &consumer)
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
