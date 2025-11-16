#include "contiki.h"

#include "udp.h"
#include "pipeline.h"

#include "net/netstack.h"
#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"

#define LOG_MODULE "[Producer]"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(main_process, "Main process");
AUTOSTART_PROCESSES(
    &main_process,
    &main_pipeline_process,
    &pipeline_process);

PROCESS_THREAD(main_process, ev, data) {
    static struct etimer timer;
    static uip_ipaddr_t root_addr;

    PROCESS_BEGIN();

    init_udp_callback();

    LOG_INFO("Waiting for sink...\n");

    while(!NETSTACK_ROUTING.node_is_reachable()) {
        etimer_set(&timer, CLOCK_SECOND * 4);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
        LOG_INFO("Still waiting for sink...\n");
    }

    NETSTACK_ROUTING.get_root_ipaddr(&root_addr);

    LOG_INFO("Sink reachable on: ");
    LOG_INFO_6ADDR(&root_addr);
    LOG_INFO_("\n");

    udp_set_sink(&root_addr);

    PROCESS_END();
}