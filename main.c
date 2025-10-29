#include "contiki.h"
#include "timeseries_data.h"
#include "sys/log.h"

#define LOG_MODULE "Mini-Project - TimeSeries"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(main_process, "Main process");
AUTOSTART_PROCESSES(&main_process);

PROCESS_THREAD(main_process, ev, data) {
    PROCESS_BEGIN();

    LOG_INFO("Printing time series data\n");

    for(unsigned int i = 0; i < timeseries_length; i++) {
        LOG_INFO_("%d ", timeseries_data[i]);

        if ((i + 1) % 15 == 0) {
            LOG_INFO_("\n");
        }
    }
    LOG_INFO_("\n");

    LOG_INFO("Done printing series.\n");

    PROCESS_END();
}