#include "pipeline.h"

#define LOG_MODULE "[Pipeline]"
#define LOG_LEVEL LOG_LEVEL_DBG

#if (LOG_LEVEL == LOG_LEVEL_DBG)
void log_packed_bytes(uint8_t *packed_buf, size_t packed_len)
{
    // Print the packed bytes (header + payload) in hex for inspection
    LOG_DBG("PKT: ");
    for (size_t b = 0; b < packed_len; ++b) {
        LOG_DBG_("%02x ", packed_buf[b]);
    }
    LOG_DBG_("\n");
}
#endif /* (LOG_LEVEL == LOG_LEVEL_DBG) */

/* ----- Process definitions -----*/
PROCESS(main_pipeline_process, "Main pipeline thread that starts the pipeline process");
PROCESS(pipeline_process, "Pipeline process");

/* Custom pipeline start event */
static process_event_t start_pipeline_event;

/* ----- Main pipeline process ----- */
PROCESS_THREAD(main_pipeline_process, ev, data)
{
    PROCESS_BEGIN();

    SENSORS_ACTIVATE(button_sensor);

    start_pipeline_event = process_alloc_event();

    LOG_INFO("Pipeline process initializer started\n");

    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

        if (!is_sink_located())
        {
            LOG_WARN("Sink has not been located, try again in few seconds!\n");
            continue;
        }

        process_post(PROCESS_BROADCAST, start_pipeline_event, NULL);
        break;
    }

    LOG_INFO("Pipeline start listener closing!\n");
    PROCESS_END();
}

/* ----- Pipeline definition ----- */
PROCESS_THREAD(pipeline_process, ev, data)
{
    static struct etimer timer;

    PROCESS_BEGIN();

    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (!(ev == start_pipeline_event))
        {
            continue;
        }

        LOG_INFO("Starting pipeline\n");
        etimer_set(&timer, CLOCK_SECOND * 0.2);

        // Buffer for packed output per block:
        // Worst-case payload = BLOCK_SIZE*BLOCK_D*16 bits = 16 bytes + 2-byte header
        // Adjust if BLOCK_SIZE or BLOCK_D changes.
        enum { MAX_W = 16 };
        static uint8_t packed_buf[2 + ((BLOCK_SIZE * BLOCK_D * MAX_W + 7) / 8)];

        static int i = 0;
        for (; i < timeseries_length; i += BLOCK_SIZE)
        {
            // Yield
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

            int n_in_block = (i + BLOCK_SIZE <= timeseries_length) ? BLOCK_SIZE : (timeseries_length - i);

            size_t packed_len = 0;
            encode(&timeseries_data[i], n_in_block, packed_buf, sizeof(packed_buf), &packed_len);

            #if (LOG_LEVEL == LOG_LEVEL_DBG)
            log_packed_bytes(packed_buf, packed_len);
            #endif

            // Send data
            send_to_sink(packed_buf, packed_len);

            etimer_reset(&timer);
        }

        LOG_INFO_("\n");
        LOG_INFO("Pipeline finished\n");

        break;
    }

    PROCESS_END();
}
/* =============================== */