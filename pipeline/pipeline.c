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
    // LOG_INFO("Sprintz\n"); // ROM space issue
    #endif /* SPRINTZ */
    #if PLA
    // LOG_INFO("PLA\n"); // ROM space issue
    #endif /* PLA */
    #if NONE
    LOG_INFO("None\n");
    #endif /* NONE */
}

// for logging energest stats
static inline uint16_t
energy_mj(uint64_t ticks, uint16_t current_01ma)
{
  /* mJ = ticks * 3V * (current_01ma / 10) / ENERGEST_SECOND */
  return (uint16_t)((ticks * 3 * current_01ma) / (((unsigned long) ENERGEST_SECOND) * 10));
}

void log_energest()
{
    energest_flush();
    
    // Energy (mJ) = (ticks / ENERGEST_SECOND) * 1000ms * voltage * current_mA / 1000
    // Simplified: (ticks * voltage * current_mA) / ENERGEST_SECOND
    uint16_t cpu_mj = energy_mj(energest_type_time(ENERGEST_TYPE_CPU), 18); // 3V * 1.8mA
    uint16_t lpm_mj = energy_mj(energest_type_time(ENERGEST_TYPE_LPM), 1); // 3V * 0.051mA
    uint16_t tx_mj = energy_mj(energest_type_time(ENERGEST_TYPE_TRANSMIT), 195); // 3V * 19.5mA
    uint16_t rx_mj = energy_mj(energest_type_time(ENERGEST_TYPE_LISTEN), 218); // 3V * 21.8mA
    
    uint16_t total_mj = (uint16_t)(cpu_mj + lpm_mj + tx_mj + rx_mj);
    
    LOG_INFO("E: cpu: %u, lpm: %u, tx: %u, rx: %u, t: %u\n", cpu_mj, lpm_mj, tx_mj, rx_mj, total_mj);
}

/* ----- Process definitions -----*/
PROCESS(main_pipeline_process, "Pipeline proc");

/* Pipeline data */
process_event_t START_PIPELINE_EVENT;

/* ----- Main pipeline process ----- */
PROCESS_THREAD(main_pipeline_process, ev, data)
{
    static struct etimer timer;
    static uint8_t packed_buf[PACKED_BUF_SIZE];
    static uint8_t seq = 0;
    static int i = 0;

    PROCESS_BEGIN();

    START_PIPELINE_EVENT = process_alloc_event();

    // Wait for configuration before starting listener
    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (ev == START_PIPELINE_EVENT)
        {
            break;
        }
        etimer_set(&timer, CLOCK_SECOND * 20);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }

    etimer_set(&timer, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    // Process execution
    while (1)
    {
        LOG_INFO("Starting pipeline\n");
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

            // Write General header
            packed_buf[0] = seq;
            packed_buf[1] = n_in_block;

            // Encode payload right after header
            size_t payload_len = 0;
            encode(&timeseries_data[i],
                   n_in_block,
                   packed_buf + HDR_LEN,
                   sizeof(packed_buf) - HDR_LEN,
                   &payload_len);
            size_t total_len = HDR_LEN + payload_len;

            #if (LOG_LEVEL == LOG_LEVEL_DBG)
            log_packed_bytes(packed_buf, total_len);
            #endif

            // Producer-side log
            LOG_INFO("TX seq=%u n=%d bytes=%u \n", (unsigned)seq, n_in_block, (unsigned)total_len);

            // Send data
            send_to_sink(packed_buf, total_len);
            // send_to_sink(packed_buf, total_len);

            seq++;
        }

        LOG_INFO("Done\n");

        log_energest();
        break;
    }

    PROCESS_END();
}
/* =============================== */