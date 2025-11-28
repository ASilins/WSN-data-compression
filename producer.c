#include "contiki.h"

#include "producer_net.h"
#include "pipeline.h"

#include "sys/log.h"

#define LOG_MODULE "[Producer]"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(main_process, "Main process");
AUTOSTART_PROCESSES(
    &main_process,
    &main_pipeline_process,
    &pipeline_process);

process_event_t CHANNEL_SETUP_EVENT;

volatile struct channel_config config;
volatile bool channel_ready = false;

PROCESS_THREAD(main_process, ev, data) {
    static struct etimer timer;
    static struct channel_config local_config;

    PROCESS_BEGIN();

    CHANNEL_SETUP_EVENT = process_alloc_event();

    start_producer_udp();

    // Send ACK to root that we have connected so it can broadcast channel switch
    while (1)
    {
        if (is_root_reachable())
        {
            break;
        }
        LOG_INFO("Could not reach root\n");
        etimer_set(&timer, CLOCK_SECOND * 4);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }
    configure_root_addr();
    etimer_set(&timer, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    send_ack_to_root();

    while (1)
    {
        PROCESS_YIELD();

        if (channel_ready)
        {
            local_config = config;
            break;
        }
    }

    etimer_set(&timer, CLOCK_SECOND * local_config.delay);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    set_channel(&local_config.ch);

    while (1)
    {
        if (is_root_reachable())
        {
            break;
        }
        LOG_INFO("Could not reach root\n");
        etimer_set(&timer, CLOCK_SECOND * 4);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }

    configure_root_addr();

    etimer_set(&timer, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    process_post(PROCESS_BROADCAST, START_PIPELINE_LISTENER_EVENT, NULL);

    PROCESS_YIELD();
    PROCESS_END();
}