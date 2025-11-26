#include "udp.h"
#include "decoder.h"

#include <inttypes.h>

#define LOG_MODULE "[Network]"
#define LOG_LEVEL LOG_LEVEL_INFO

static struct simple_udp_connection udp_conn;
static struct simple_udp_connection channel_udp_conn;
static uip_ipaddr_t *sink_addr;

static void
udp_rx_callback(struct simple_udp_connection *c,
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

    LOG_DBG("Data:");
    for (uint16_t i = 0; i < datalen; i++) {
        LOG_DBG_("%02X ", data[i]);
    }
    LOG_DBG_("\n");
}

static void
channel_selection_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr,
                uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr,
                uint16_t receiver_port,
                const uint8_t *data,
                uint16_t datalen)
{
    if (datalen == 1) {
        int channel = data[0];
        LOG_INFO("Setting channel to: %d\n", channel);
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel);
        rpl_timers_schedule_periodic_dis();
    }
}

void init_udp_callback()
{
    LOG_INFO("Setting up UDP callback\n");
    simple_udp_register(&channel_udp_conn, CHANNEL_BROADCAST_PORT, NULL,
        CHANNEL_BROADCAST_PORT, channel_selection_rx_callback);
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
        UDP_SERVER_PORT, udp_rx_callback);
}

bool is_sink_located()
{
    return sink_addr != NULL;
}

void udp_set_sink(uip_ipaddr_t *addr)
{
    sink_addr = addr;
}

void send_to_sink(uint8_t *data, size_t len)
{
    static uint32_t tx_count;

    if (sink_addr != NULL) {
        LOG_DBG("Sending request %"PRIu32" to ", tx_count);
        LOG_DBG_6ADDR(sink_addr);
        LOG_DBG_("\n");
        simple_udp_sendto(&udp_conn, data, len, sink_addr);
        tx_count++;
    } else {
        LOG_WARN("Sink not reachable yet\n");
    }
}