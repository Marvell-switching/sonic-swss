#ifndef __SCHEDULERMGR__
#define __SCHEDULERMGR__

#include "dbconnector.h"
#include "producerstatetable.h"
#include "orch.h"

#include <map>
#include <string>

namespace swss {

#define TIME_RANGE_ENABLED_STR "enabled"
#define TIME_RANGE_DISABLED_STR "disabled"
#define TIME_RANGE_STATUS_STR "status"

#define SYSTEMD_FILES_PATH_PREFIX_STR "/etc/systemd/system/"
#define SYSTEMD_UNIT_FILE_EXTENSION_STR ".service"
#define SYSTEMD_TIMER_FILE_EXTENSION_STR ".timer"
#define SYSTEMD_RELOAD_CMD_STR "systemctl daemon-reload"
#define SYSTEMD_TIMER_ENABLE_PREFIX_CMD_STR "systemctl enable "

class SchedulerMgr : public Orch
{
public:
    SchedulerMgr(DBConnector *cfgDb, DBConnector *stateDb, const std::vector<std::string> &tableNames);
    using Orch::doTask;

private:

    Table m_cfgTimeRangeTable;
    Table m_cfgScheduledConfigurationTable;

    ProducerStateTable m_stateTimeRangeStatusTable;

    bool reloadSystemd();
    bool enableSystemdTimer(const std::string& timerFileName);
    task_process_status writeSystemdUnitFile(const std::string& taskName, const std::string& taskType, const std::string& taskCommand);
    task_process_status writeSystemdTimerFile(const std::string& taskName, const std::string& taskOnCalendar, const bool isPersistent);
    task_process_status createSystemdFiles(const std::string& rangeName, const std::string& start, const std::string& end);
    task_process_status doTimeRangeTask(const std::string& rangeName, const std::vector<FieldValueTuple>& fieldValues);
    void doTask(Consumer &consumer);
};

}

#endif /* __SCHEDULERMGR__ */
