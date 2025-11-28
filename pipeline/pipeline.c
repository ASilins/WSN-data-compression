#include "pipeline.h"

#define LOG_MODULE "[Pipeline]"
#define LOG_LEVEL LOG_LEVEL_INFO

#if (LOG_LEVEL == LOG_LEVEL_DBG)
void log_packed_bytes(uint8_t *packed_buf, size_t packed_len)
{
    /* Print the packed bytes (header + payload) 
    ** in hex for inspection. */
    LOG_DBG("PKT: ");
    for (size_t b = 0; b < packed_len; ++b) {
        LOG_DBG_("%02x ", packed_buf[b]);
    }
    LOG_DBG_("\n");
}
#endif /* (LOG_LEVEL == LOG_LEVEL_DBG) */

void log_algo()
{
    #if SPRINTZ
    LOG_INFO("Sprintz\n");
    #endif /* SPRINTZ */
    #if NONE
    LOG_INFO("None\n");
    #endif /* NONE */
}

// for logging energest stats
void log_energest_stats()
{
    energest_flush();

    unsigned long energest_second = ENERGEST_SECOND;
    
    uint64_t cpu_time = energest_type_time(ENERGEST_TYPE_CPU);
    uint64_t lpm_time = energest_type_time(ENERGEST_TYPE_LPM);
    uint64_t tx_time = energest_type_time(ENERGEST_TYPE_TRANSMIT);
    uint64_t rx_time = energest_type_time(ENERGEST_TYPE_LISTEN);
    
    // Energy (mJ) = (ticks / ENERGEST_SECOND) * 1000ms * voltage * current_mA / 1000
    // Simplified: (ticks * voltage * current_mA) / ENERGEST_SECOND
    
    uint16_t cpu_mj = (uint16_t)((cpu_time * 3 * 18) / (energest_second * 10)); // 3V * 1.8mA
    uint16_t lpm_mj = (uint16_t)((lpm_time * 3 * 51) / (energest_second * 1000)); // 3V * 0.051mA
    uint16_t tx_mj = (uint16_t)((tx_time * 3 * 195) / (energest_second * 10)); // 3V * 19.5mA
    uint16_t rx_mj = (uint16_t)((rx_time * 3 * 218) / (energest_second * 10)); // 3V * 21.8mA
    
    uint16_t total_mj = (uint16_t)(cpu_mj + lpm_mj + tx_mj + rx_mj);
    
    LOG_INFO("E:%u,%u,%u,%u,%u\n", cpu_mj, lpm_mj, tx_mj, rx_mj, total_mj);
}

/* ----- Process definitions -----*/
PROCESS(main_pipeline_process, "Main pipeline thread that starts the pipeline process");
PROCESS(pipeline_process, "Pipeline process");

/* Pipeline data */
process_event_t START_PIPELINE_EVENT;
process_event_t START_PIPELINE_LISTENER_EVENT;
static bool pipeline_running = false;

/* ----- Main pipeline process ----- */
PROCESS_THREAD(main_pipeline_process, ev, data)
{
    PROCESS_BEGIN();

    START_PIPELINE_EVENT = process_alloc_event();
    START_PIPELINE_LISTENER_EVENT = process_alloc_event();

    // Wait for configuration before starting listener
    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (ev == START_PIPELINE_LISTENER_EVENT)
        {
            break;
        }
    }

    SENSORS_ACTIVATE(button_sensor);

    LOG_INFO("Ready\n");

    while (1)
    {
        PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

        if (pipeline_running)
        {
            continue;
        }

        process_post(PROCESS_BROADCAST, START_PIPELINE_EVENT, NULL);
        break;
    }

    PROCESS_END();
}

/* ----- Pipeline definition ----- */
PROCESS_THREAD(pipeline_process, ev, data)
{
    static struct etimer timer;
    static uint8_t packed_buf[PACKED_BUF_SIZE];
    static uint8_t seq = 0;
    static int i = 0;

    PROCESS_BEGIN();

    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (!(ev == START_PIPELINE_EVENT))
        {
            continue;
        }

        LOG_INFO("Start\n");
        pipeline_running = true;
        i = 0;
        seq = 0;
        log_algo();

        energest_flush();
        for (; i < timeseries_length; i += BLOCK_SIZE)
        {
            // Yield
            etimer_set(&timer, CLOCK_SECOND / 20);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

            uint8_t n_in_block = (i + BLOCK_SIZE <= timeseries_length)
                ? (uint8_t) BLOCK_SIZE
                : (uint8_t) (timeseries_length - i);

            // Write header
            packed_buf[0] = seq;
            packed_buf[1] = n_in_block;

            // Encode payload right after header
            size_t payload_len = 0;
            encode(&timeseries_data[i],
                   n_in_block,
                   packed_buf + HDR_LEN,
                   sizeof(packed_buf) - HDR_LEN,
                   &payload_len);
            payload_len += HDR_LEN;

            #if (LOG_LEVEL == LOG_LEVEL_DBG)
            log_packed_bytes(packed_buf, payload_len);

            // Producer-side log
            LOG_DBG("TX seq=%u n=%d bytes=%u \n", (unsigned)seq, n_in_block, (unsigned)payload_len);
            #endif

            // Send data
            send_to_sink(packed_buf, payload_len);

            seq++;
        }

        LOG_INFO("Done\n");

        log_energest_stats();
        pipeline_running = false;

    }


    PROCESS_END();
}
/* =============================== */