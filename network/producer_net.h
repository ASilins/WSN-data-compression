#ifndef PRODUCER_NET_H
#define PRODUCER_NET_H

#include "project-conf.h"
#include "contiki.h"

#include <stdint.h>
#include <stddef.h>

#include "decoder.h"
#include "net/routing/rpl-lite/rpl.h"

#include "net/netstack.h"
#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"

/* -------------------------------------------- */
struct channel_config {
    uint8_t ch;
    uint8_t delay;
};

extern process_event_t CHANNEL_SETUP_EVENT;

extern volatile struct channel_config config;
extern volatile bool channel_ready;

void set_channel(uint8_t *ch);

void start_producer_udp();

bool is_root_reachable();
void send_ack_to_root();
void configure_root_addr();

void send_to_sink(uint8_t *data, size_t len);
#endif /* PRODUCER_NET_H */