#include "contiki.h"
#include "project-conf.h"

#include "root_net.h"

#include "sys/log.h"

#define LOG_MODULE "[Sink]"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(main_process, "Main process");
AUTOSTART_PROCESSES(&main_process);

volatile bool mote_ready = false;

PROCESS_THREAD(main_process, ev, data) {
    static struct etimer timer;
    static struct channel_config config;
    static uint8_t i = 0;

    PROCESS_BEGIN();

    // Run channel selection
    config.ch = channel_selection();

    etimer_set(&timer, CLOCK_SECOND * 1);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    // Start DAG root
    start_dag_root();

    // Allow for root broadcast message
    etimer_set(&timer, CLOCK_SECOND * 1);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    LOG_INFO("Waiting for DAG root discovery\n");

    while (1)
    {
        PROCESS_YIELD();

        if (mote_ready)
        {
            etimer_set(&timer, CLOCK_SECOND / 2);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
            break;
        }
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

    PROCESS_YIELD();
    PROCESS_END();
}