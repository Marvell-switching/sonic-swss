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

SchedulerMgr::SchedulerMgr(DBConnector *cfgDb, DBConnector *stateDb, string pg_lookup_file, const vector<string> &tableNames) :
        Orch{cfgDb, tableNames},
        m_cfgTimeRangeTable{cfgDb, CFG_TIME_RANGE_TABLE_NAME},
        m_cfgScheduledConfigurationTable{cfgDb, CFG_SCHEDULED_CONFIGURATION_TABLE_NAME},
        m_stateTimeRangeStatusTable{stateDb, STATE_TIME_RANGE_STATUS_TABLE_NAME}
{}

void writeSystemdUnitFile(const string& fileNameWithoutExtension, const string& taskType, const string& taskCommand) {
    string unitFileName = fileNameWithoutExtension + ".service";
    ofstream unitFile(unitFileName);
    unitFile << "[Unit]\n";
    unitFile << "Description=" << fileNameWithoutExtension << " service\n\n";
    unitFile << "[Service]\n";
    unitFile << "Type=" << taskType << "\n";
    unitFile << "ExecStart=" << taskCommand << "\n";
    unitFile.close();

    SWSS_LOG_DEBUG("Systemd unit file for %s has been created", fileNameWithoutExtension.c_str());
}

void writeSystemdTimerFile(const string& fileNameWithoutExtension, const string& taskOnCalendar, bool isPersistent) {
    string timerFileName = fileNameWithoutExtension + ".timer";
    ofstream timerFile(timerFileName);
    timerFile << "[Unit]\n";
    timerFile << "Description=Timer for " << fileNameWithoutExtension << " service\n\n";
    timerFile << "[Timer]\n";
    timerFile << "OnCalendar=" << taskOnCalendar << "\n";
    if (isPersistent) {
        timerFile << "Persistent=true\n";
    }
    timerFile << "\n"
    timerFile << "[Install]\n";
    timerFile << "WantedBy=timers.target\n";
    timerFile.close();

    SWSS_LOG_DEBUG("Systemd timer file for %s has been created", fileNameWithoutExtension.c_str());
}

void writeSystemdFiles(const string& taskName, const string& startTime, const string& endTime) {
    // Service file for enabling the task
    string enableServiceFileName = taskName + "-enable";
    writeSystemdUnitFile(enableServiceFileName, "oneshot", "/bin/echo 'Starting " + taskName + "'"); // TODO Replace with actual command 

    // Timer file for enabling the task
    string enableTimerFileName = taskName + "-enable";
    writeSystemdTimerFile(enableTimerFileName, startTime, false);

    // Service file for disabling the task
    string disableServiceFileName = taskName + "-disable";
    writeSystemdUnitFile(disableServiceFileName, "oneshot", "/bin/echo 'Stopping " + taskName + "'"); // TODO Replace with actual command

    // Timer file for disabling the task
    string disableTimerFileName = taskName + "-disable";
    writeSystemdTimerFile(disableTimerFileName, endTime, false);

    SWSS_LOG_INFO("Systemd files for %s have been created", taskName.c_str())
}


task_process_status SchedulerMgr::doTimeRangeTask(string rangeName, vector<FieldValueTuple> fieldValues){
    SWSS_LOG_ENTER();
    string start = "";
    string end = "";

    for (auto i : fieldValues)
    {
        if (fvField(i) == "start"){
            start = fvValue(i);
        } else if(fvField(i) == "end"){
            end = fvValue(i);
        } else{
            SWSS_LOG_ERROR("Time range %s has unknown field %s", rangeName.c_str(), fvField(i).c_str());
            // Can skip instead of returning invalid entry
            return task_process_status::task_invalid_entry;
        }
    }

    if (start == "" || end == ""){
        SWSS_LOG_ERROR("Time range %s is missing start or end time", rangeName.c_str());
        return task_process_status::task_invalid_entry;
    }

    // Create systemd files for time range
    writeSystemdFiles(rangeName, start, end);

    // Add time range status to range status table in state db
    m_stateTimeRangeStatusTable.set(rangeName, TIME_RANGE_DISABLED);

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
                doTimeRangeTask(rangeName, kfvFieldsValues(t));
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
