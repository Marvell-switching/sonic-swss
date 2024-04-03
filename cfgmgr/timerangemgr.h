#ifndef __TIMERANGEMGR__
#define __TIMERANGEMGR__

#include "dbconnector.h"
#include "producerstatetable.h"
#include "orch.h"

#include <map>
#include <string>
#include <table.h>
#include <vector>

namespace swss {

#define TIME_RANGE_ENABLED_STR "enabled"
#define TIME_RANGE_DISABLED_STR "disabled"
#define TIME_RANGE_STATUS_STR "status"


#define CRON_FILES_PATH_PREFIX_STR "/etc/cron.d/"
#define CRON_USERNAME_STR "root"


class TimeRangeMgr : public Orch
{
public:
    TimeRangeMgr(DBConnector *cfgDb, DBConnector *stateDb, const std::vector<std::string> &tableNames);
    using Orch::doTask;

private:
    Table m_stateTimeRangeStatusTable;

    task_process_status writeCrontabFile(const std::string& fileName, const std::string& schedule, const std::string& command, bool deleteSelfAfterCompletion);
    task_process_status createCronjobs(const std::string& rangeName, const std::string& start, const std::string& end, bool runOnce);
    task_process_status doTimeRangeTask(const std::string& rangeName, const std::vector<FieldValueTuple>& fieldValues);
    void doTask(Consumer &consumer);
};

}

#endif /* __TIMERANGEMGR__ */
