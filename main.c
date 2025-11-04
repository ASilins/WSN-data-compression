#include <stdlib.h>

#include "contiki.h"
#include "timeseries_data.h"
#include "sys/log.h"

#define LOG_MODULE "Mini-Project - TimeSeries"
#define LOG_LEVEL LOG_LEVEL_INFO

void delta_encoding(const int16_t* timeseries_data, int16_t* deltas, unsigned int length) {
    deltas[0] = timeseries_data[0];
    
    for(unsigned int i = 1; i < length; i++) {
        deltas[i] = timeseries_data[i] - timeseries_data[i - 1];
    }
}

PROCESS(main_process, "Main process");
AUTOSTART_PROCESSES(&main_process);

PROCESS_THREAD(main_process, ev, data) {
    PROCESS_BEGIN();

    int16_t* deltas = malloc(timeseries_length * sizeof(int16_t));
    delta_encoding(timeseries_data, deltas, timeseries_length);

    // Print the differenced data
    LOG_INFO("Printing deltas\n");
    for(unsigned int i = 0; i < timeseries_length; i++) {
        LOG_INFO_("%d ", deltas[i]);

        if ((i + 1) % 15 == 0) {
            LOG_INFO_("\n");
        }
    }
    LOG_INFO_("\n");

    LOG_INFO("Done printing deltas.\n");

    free(deltas);

    PROCESS_END();
}