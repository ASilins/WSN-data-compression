#ifndef ROOT_NET_H
#define ROOT_NET_H

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

extern volatile bool mote_ready;

void set_channel(uint8_t *ch);

void start_dag_root();
void channel_switch();
void broadcast_channel_config(struct channel_config *config);
uint8_t channel_selection();

/* ============================================ */
#endif /* ROOT_NET_H */