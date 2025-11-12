#include "contiki.h"
#include "udp.h"
#include "sys/log.h"

#define LOG_MODULE "[Sink]"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(main_process, "Main process");
AUTOSTART_PROCESSES(&main_process);

PROCESS_THREAD(main_process, ev, data) {
    PROCESS_BEGIN();

    init_udp_root_callback();

    PROCESS_END();
}