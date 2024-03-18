#ifndef __SCHEDULERMGR__
#define __SCHEDULERMGR__

#include "dbconnector.h"
#include "producerstatetable.h"
#include "orch.h"

#include <map>
#include <string>

namespace swss {

#define TIME_RANGE_ENABLED "enabled"
#define TIME_RANGE_DISABLED "disabled"

class SchedulerMgr : public Orch
{
public:
    SchedulerMgr(DBConnector *cfgDb, DBConnector *stateDb, const std::vector<std::string> &tableNames);
    using Orch::doTask;

private:

    Table m_cfgTimeRangeTable;
    Table m_cfgScheduledConfigurationTable;

    ProducerStateTable m_stateTimeRangeStatusTable;
    
    void writeSystemdUnitFile(string taskName, string taskType, string taskCommand);
    void writeSystemdTimerFile(string taskName, string taskOnCalendar, bool isPersistent);
    void writeSystemdFiles(string rangeName, string start, string end);
    doTimeRangeTask doTimeRangeTask(string rangeName, vector<FieldValueTuple> &fieldValues);
    void doTask(Consumer &consumer);
};

}

#endif /* __SCHEDULERMGR__ */
