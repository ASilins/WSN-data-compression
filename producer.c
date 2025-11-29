#include "contiki.h"

#include "producer_net.h"
#include "pipeline.h"

#include "sys/log.h"

#define LOG_MODULE "[Producer]"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(main_process, "Main");
AUTOSTART_PROCESSES(
    &main_process,
    &main_pipeline_process);

process_event_t CHANNEL_SETUP_EVENT;

volatile struct channel_config config;
volatile bool channel_ready = false;

PROCESS_THREAD(main_process, ev, data) {
    static struct etimer timer;
    static struct channel_config local_config;
    static uint8_t i = 0;

    PROCESS_BEGIN();

    CHANNEL_SETUP_EVENT = process_alloc_event();

    start_producer_udp();

    // Send ACK to root that we have connected so it can broadcast channel switch
    while (1)
    {
        if (root_is_known())
        {
            break;
        }
        etimer_set(&timer, CLOCK_SECOND / 2);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }

    etimer_set(&timer, CLOCK_SECOND * 5);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    LOG_INFO("Sending ACK\n");
    for (; i < 5; i++)
    {
        send_ack_to_root();
        etimer_set(&timer, CLOCK_SECOND / 2);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }

    while (1)
    {
        if (channel_ready)
        {
            local_config = config;
            break;
        }
        etimer_set(&timer, CLOCK_SECOND * 2);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

        PROCESS_YIELD();
    }

    etimer_set(&timer, CLOCK_SECOND * local_config.delay);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    LOG_INFO("New channel: %d\n", local_config.ch);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, local_config.ch);

    while (1)
    {
        if (root_is_known())
        {
            break;
        }
        etimer_set(&timer, CLOCK_SECOND * 1);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }

    etimer_set(&timer, CLOCK_SECOND * 5);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    process_post(PROCESS_BROADCAST, START_PIPELINE_EVENT, NULL);

    PROCESS_YIELD();
    PROCESS_END();
}