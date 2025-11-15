#ifndef SINK_NET_H
#define SINK_NET_H

#include "project-conf.h"

#include <stdint.h>
#include <stddef.h>

#include "net/netstack.h"
#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"

#include "decoder.h"

#include "sys/log.h"

/* -------------------------------------------- */

void init_sink_udp();

/* ============================================ */
#endif