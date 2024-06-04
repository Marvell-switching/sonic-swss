#ifndef __TIMERANGEMGR__
#define __TIMERANGEMGR__

#include "dbconnector.h"
#include "producerstatetable.h"
#include "orch.h"

#include <map>
#include <string>
#include <table.h>
#include <vector>
#include "croncpp.h"


namespace swss {

#define TIME_RANGE_ACTIVE_STR "active"
#define TIME_RANGE_INACTIVE_STR "inactive"
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

    task_process_status writeCrontabFile(const std::string& fileName, const std::string& schedule, const std::string& command);
    task_process_status createCronjobs(const std::string& rangeName, const std::string& start, const std::string& end, const std::string& startYear, const std::string& endYear);
    bool isTimeInRange(const cron::cronexpr& startExpr, const cron::cronexpr& endExpr, const std::tm& currentTM, const std::string& startYear, const std::string& endYear);
    bool isTimeInRange(const cron::cronexpr& startExpr, const cron::cronexpr& endExpr, const std::time_t& currentTime, const std::string& startYear, const std::string& endYear);
    
    void doTask(Consumer &consumer);
    task_process_status doTimeRangeTask(const std::string& rangeName, const std::vector<FieldValueTuple>& fieldValues);
    task_process_status doTimeRangeTaskDelete(const std::string &rangeName);
};

}

#endif /* __TIMERANGEMGR__ */
