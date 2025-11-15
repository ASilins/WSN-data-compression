#include "sink_net.h"

#define LOG_MODULE "[Network]"
#define LOG_LEVEL LOG_LEVEL_DBG

static struct simple_udp_connection sink_udp_conn;

static void
udp_rx_sink_decode_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
    LOG_DBG("Received %u bytes from ", datalen);
    LOG_DBG_6ADDR(sender_addr);
    LOG_DBG_("\n");

    decode(data, datalen);
}

void init_sink_udp()
{
    LOG_INFO("Configuring sink UDP connection");

    /* Initialize DAG root */
    NETSTACK_ROUTING.root_start();

    simple_udp_register(&sink_udp_conn, UDP_SERVER_PORT, NULL,
        UDP_CLIENT_PORT, udp_rx_sink_decode_callback);
}