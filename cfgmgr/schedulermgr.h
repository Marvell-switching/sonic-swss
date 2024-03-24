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


#define CRON_FILES_PATH_PREFIX_STR "/etc/cron.d/"
#define CRON_USERNAME_STR "root"


class SchedulerMgr : public Orch
{
public:
    SchedulerMgr(DBConnector *cfgDb, DBConnector *stateDb, const std::vector<std::string> &tableNames);
    using Orch::doTask;

private:

    Table m_cfgTimeRangeTable;
    Table m_cfgScheduledConfigurationTable;

    ProducerStateTable m_stateTimeRangeStatusTable;

    task_process_status writeCrontabFile(const std::string& fileName, const std::string& schedule, const std::string& command, bool deleteSelfAfterCompletion);
    task_process_status createCronjobs(const std::string& rangeName, const std::string& start, const std::string& end, bool runOnce);
    task_process_status doTimeRangeTask(const std::string& rangeName, const std::vector<FieldValueTuple>& fieldValues);
    void doTask(Consumer &consumer);
};

}

#endif /* __SCHEDULERMGR__ */
