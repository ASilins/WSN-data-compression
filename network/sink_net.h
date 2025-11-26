#ifndef SINK_NET_H
#define SINK_NET_H

#include "contiki.h"
#include "project-conf.h"

#include <stdint.h>
#include <stddef.h>

#include "net/netstack.h"
#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"
#include "net/routing/rpl-lite/rpl.h"

#include "decoder.h"

#include "sys/log.h"

/* -------------------------------------------- */

extern struct process channel_selection_process;

void init_sink_udp();

/* ============================================ */
#endif