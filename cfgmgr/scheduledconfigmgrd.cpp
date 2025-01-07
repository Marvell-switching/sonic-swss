#include <unistd.h>
#include <vector>
#include <mutex>
#include "dbconnector.h"
#include "select.h"
#include "exec.h"
#include "schema.h"
#include "scheduledconfigmgr.h"
#include <fstream>
#include <iostream>

using namespace std;
using namespace swss;

/* SELECT() function timeout retry time, in millisecond */
#define SELECT_TIMEOUT 1000

int main(int argc, char **argv)
{
    Logger::linkToDbNative("scheduledconfigmgrd");
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("--- Starting scheduledconfigmgrd ---");

    try
    {
        std::vector<Orch *> cfgOrchList;

        // Create DB connectors
        DBConnector cfgDb("CONFIG_DB", 0);
        DBConnector stateDb("STATE_DB", 0);
        DBConnector dynDb("DYNAMIC_DB", 0);
        
        // Create table connectors that ScheduledConfigMgr will subscribe to
        TableConnector cfgDbScheduledConfigurations(&cfgDb, CFG_SCHEDULED_CONFIGURATION_TABLE_NAME);
        TableConnector stateDbTimeRangeStatusTable(&stateDb, STATE_TIME_RANGE_STATUS_TABLE_NAME);
        vector<TableConnector> connectors = {cfgDbScheduledConfigurations, stateDbTimeRangeStatusTable};

        cfgOrchList.emplace_back(new ScheduledConfigMgr(connectors, &dynDb));

        auto scheduledconfigmgr = cfgOrchList[0];

        swss::Select s;
        for (Orch *o : cfgOrchList)
        {
            s.addSelectables(o->getSelectables());
        }

        SWSS_LOG_NOTICE("starting main loop");
        while (true)
        {
            Selectable *sel;
            int ret;

            ret = s.select(&sel, SELECT_TIMEOUT);
            if (ret == Select::ERROR)
            {
                SWSS_LOG_NOTICE("Error: %s!", strerror(errno));
                continue;
            }
            if (ret == Select::TIMEOUT)
            {
                scheduledconfigmgr->doTask();
                continue;
            }

            auto *c = (Executor *)sel;
            c->execute();
        }
    }
    catch(const std::exception &e)
    {
        SWSS_LOG_ERROR("Runtime error: %s", e.what());
    }
    return -1;
}
