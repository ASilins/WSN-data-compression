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
PROCESS(main_pipeline_process, "Pipeline proc");

/* Pipeline data */
process_event_t START_PIPELINE_EVENT;

/* ----- Main pipeline process ----- */
PROCESS_THREAD(main_pipeline_process, ev, data)
{
    static struct etimer timer;
    //static uint8_t packed_buf[PACKED_BUF_SIZE];
    //static uint8_t seq = 0;
    //static int i = 0;

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

    #ifdef SPRINTZ
        /* Sprintz header layout:
        ** [0..1]   uint16_t seq
        ** [2..3]   uint16_t n_in_block
        ** [4..5]   int16_t  prev_sample (for BLOCK_D == 1) */
        enum { SPRINTZ_HDR_LEN = 2 + 2 + 2 };
        enum { MAX_W = 16 };

        static uint8_t packed_buf[SPRINTZ_HDR_LEN + 
            ((BLOCK_SIZE * BLOCK_D * MAX_W + 7) / 8)];
    #else
        /* PLA header layout (matches Sprintz for decoder compatibility):
        ** [0..1]   uint16_t seq
        ** [2..3]   uint16_t n_in_block
        ** [4..5]   int16_t prev_sample (unused for PLA, set to 0)
        ** [6+]     encoder output (num_segments + segments)
        ** Payload: 5 bytes per segment (end_index, start_value, slope_q) */
        enum { PLA_HDR_LEN = 6 };
            
        /* Worst case: each sample is its own segment = timeseries_length segments
        ** Each segment = 5 bytes, +1 for num_segments byte */
        static uint8_t packed_buf[PLA_HDR_LEN + 1 + (BLOCK_SIZE * 5)];
    #endif

        static uint16_t seq = 0;

        static int i = 0;
        for (; i < timeseries_length; i += BLOCK_SIZE)
        {
            // Yield
            etimer_set(&timer, CLOCK_SECOND / 20);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

            uint8_t n_in_block = (i + BLOCK_SIZE <= timeseries_length)
                ? (uint8_t) BLOCK_SIZE
                : (uint8_t) (timeseries_length - i);

    #ifdef SPRINTZ
            int16_t prev_sample = (i == 0) ? 0 : timeseries_data[i - 1];

            // Write Sprintz header (little-endian)
            packed_buf[0] = (uint8_t)(seq & 0xff);
            packed_buf[1] = (uint8_t)((seq >> 8) & 0xff);
            packed_buf[2] = (uint8_t)(n_in_block & 0xff);
            packed_buf[3] = (uint8_t)((n_in_block >> 8) & 0xff);
            packed_buf[4] = (uint8_t)(prev_sample & 0xff);
            packed_buf[5] = (uint8_t)((prev_sample >> 8) & 0xff);

            // Encode payload right after header
            size_t payload_len = 0;
            encode(&timeseries_data[i],
                   n_in_block,
                   packed_buf + SPRINTZ_HDR_LEN,
                   sizeof(packed_buf) - SPRINTZ_HDR_LEN,
                   &payload_len);
                   
            payload_len += HDR_LEN;
            size_t total_len = SPRINTZ_HDR_LEN + payload_len;

            #if (LOG_LEVEL == LOG_LEVEL_DBG)
            log_packed_bytes(packed_buf, payload_len);
            #endif

            LOG_INFO("TX seq=%u n=%d bytes=%u prev=%d\n",
                     (unsigned)seq, n_in_block, (unsigned)total_len, (int)prev_sample);
    #else
            // Write PLA header (little-endian) - 6 bytes to match Sprintz format
            packed_buf[0] = (uint8_t)(seq & 0xff);
            packed_buf[1] = (uint8_t)((seq >> 8) & 0xff);
            packed_buf[2] = (uint8_t)(n_in_block & 0xff);
            packed_buf[3] = (uint8_t)((n_in_block >> 8) & 0xff);
            packed_buf[4] = 0;  // prev_sample low byte (unused for PLA)
            packed_buf[5] = 0;  // prev_sample high byte (unused for PLA)

            // PLA encoding - encode after header
            size_t payload_len = 0;
            encode(&timeseries_data[i],
                   n_in_block,
                   packed_buf + PLA_HDR_LEN,
                   sizeof(packed_buf) - PLA_HDR_LEN,
                   &payload_len);

            size_t total_len = PLA_HDR_LEN + payload_len;

            #if (LOG_LEVEL == LOG_LEVEL_DBG)
            log_packed_bytes(packed_buf, total_len);
            #endif

            // PLA log (number of segments is in packed_buf[PLA_HDR_LEN])
            uint8_t num_segments = packed_buf[PLA_HDR_LEN];
            LOG_INFO("TX seq=%u n=%d bytes=%u segments=%u\n",
                     (unsigned)seq, n_in_block, (unsigned)total_len, num_segments);
    #endif

            // Send data
            send_to_sink(packed_buf, total_len);

            seq++;
        }

        LOG_INFO("Done\n");

        log_energest_stats();
        break;
    }

    PROCESS_END();
}
/* =============================== */