#include "contiki.h"
#include "project-conf.h"

#include "root_net.h"

#include "sys/log.h"
#include "sys/energest.h"

#define LOG_MODULE "[Sink]"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(main_process, "Main process");
AUTOSTART_PROCESSES(&main_process);

volatile bool mote_ready = false;

// for logging energest stats
void log_energest_stats()
{
    energest_flush();

    unsigned long energest_second = ENERGEST_SECOND;
    
    uint64_t cpu_time = energest_type_time(ENERGEST_TYPE_CPU);
    uint64_t lpm_time = energest_type_time(ENERGEST_TYPE_LPM);
    uint64_t tx_time = energest_type_time(ENERGEST_TYPE_TRANSMIT);
    uint64_t rx_time = energest_type_time(ENERGEST_TYPE_LISTEN);
    
    uint16_t cpu_mj = (uint16_t)((cpu_time * 3 * 18) / (energest_second * 10)); // 3V * 1.8mA
    uint16_t lpm_mj = (uint16_t)((lpm_time * 3 * 51) / (energest_second * 1000)); // 3V * 0.051mA
    uint16_t tx_mj = (uint16_t)((tx_time * 3 * 195) / (energest_second * 10)); // 3V * 19.5mA
    uint16_t rx_mj = (uint16_t)((rx_time * 3 * 218) / (energest_second * 10)); // 3V * 21.8mA
    
    uint16_t total_mj = cpu_mj + lpm_mj + tx_mj + rx_mj;
    
    LOG_INFO("E: cpu: %u, lpm: %u, tx: %u, rx: %u, total: %u\n", cpu_mj, lpm_mj, tx_mj, rx_mj, total_mj);
}

PROCESS_THREAD(main_process, ev, data) {
    static struct etimer timer;
    static struct channel_config config;
    static uint8_t i = 0;

    PROCESS_BEGIN();

    // Flush energest at start
    energest_flush();

    // Run channel selection
    config.ch = channel_selection();

    etimer_set(&timer, CLOCK_SECOND * 1);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    // Start DAG root
    start_dag_root();

    // Allow for root broadcast message
    etimer_set(&timer, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    LOG_INFO("Waiting for DAG root discovery\n");

    while (1)
    {
        PROCESS_YIELD();

        if (mote_ready)
        {
            etimer_set(&timer, CLOCK_SECOND * 12);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
            break;
        }
        etimer_set(&timer, CLOCK_SECOND / 4);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }

    // Broadcast channel selection
    LOG_INFO("Running channel switch broadcast\n");
    for (; i < CHANNEL_BROADCAST_COUNT; i++)
    {
        config.delay = (uint8_t) (CHANNEL_BROADCAST_COUNT * CHANNEL_BROADCAST_DELAY - i);
        broadcast_channel_config(&config);
        etimer_set(&timer, CLOCK_SECOND * CHANNEL_BROADCAST_DELAY);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }

    // Set root nodes channel
    set_channel(&config.ch);
    channel_switch();
    
    // Log energy consumption after all operations complete
    LOG_INFO("Operations complete\n");
    log_energest_stats();

    PROCESS_YIELD();
    PROCESS_END();
}