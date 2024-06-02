#include <unistd.h>
#include <vector>
#include <mutex>
#include "dbconnector.h"
#include "select.h"
#include "exec.h"
#include "schema.h"
#include "timerangemgr.h"
#include <fstream>
#include <iostream>

using namespace std;
using namespace swss;

/* SELECT() function timeout retry time, in millisecond */
#define SELECT_TIMEOUT 1000

int main(int argc, char **argv)
{
    Logger::linkToDbNative("timerangemgrd");
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("--- Starting timerangemgrd ---");

    try
    {
        std::vector<Orch *> cfgOrchList;

        DBConnector cfgDb("CONFIG_DB", 0);
        DBConnector stateDb("STATE_DB", 0);

        vector<string> cfg_buffer_tables = {
            CFG_TIME_RANGE_TABLE_NAME,
        };
        cfgOrchList.emplace_back(new TimeRangeMgr(&cfgDb, &stateDb, cfg_buffer_tables));

        auto timerangemgr = cfgOrchList[0];

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
                timerangemgr->doTask();
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