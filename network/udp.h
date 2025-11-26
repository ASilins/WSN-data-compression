#ifndef UDP_H
#define UDP_H

#include "project-conf.h"

#include <stdint.h>
#include <stddef.h>

#include "net/netstack.h"
#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"
#include "net/routing/rpl-lite/rpl.h"

#include "sys/log.h"

/* -------------------------------------------- */

void init_udp_callback();

void udp_set_sink(uip_ipaddr_t *addr);
void send_to_sink(uint8_t *data, size_t len);

bool is_sink_located();

/* ============================================ */

#endif /* UDP_H */