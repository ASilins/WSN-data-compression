#include "producer_net.h"

/* -------------------------------------------------------------------- */
#define LOG_MODULE "[Network]"
#define LOG_LEVEL LOG_LEVEL_INFO

/* -------------------------------------------------------------------- */
static struct simple_udp_connection root_ack_conn;
static struct simple_udp_connection udp_conn;
static struct simple_udp_connection channel_selection_conn;
static uip_ipaddr_t root_addr;
static uint32_t tx_count;
extern struct process main_process;

/* -------------------------------------------------------------------- */
static void
udp_rx_channel_selection_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen)
{
    if (datalen == 2)
    {
        config.ch = (uint8_t) data[0];
        config.delay = (uint8_t) data[1];
        channel_ready = true;

        // De-register channel selection listener
        simple_udp_register(&channel_selection_conn, CHANNEL_BROADCAST_PORT, NULL,
            CHANNEL_BROADCAST_PORT, NULL);

        process_poll(&main_process);
    } else {
        LOG_ERR("Received wrong data\n");
    }
}

/* -------------------------------------------------------------------- */
void set_channel(uint8_t *ch)
{
    LOG_INFO("Switching channel: %d\n", *ch);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, *ch);
}

void start_producer_udp()
{
    LOG_INFO("Starting UDP\n");
    simple_udp_register(&root_ack_conn, ROOT_ACK_PORT, NULL,
        ROOT_ACK_PORT, NULL);
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
        UDP_SERVER_PORT, NULL);
    simple_udp_register(&channel_selection_conn, CHANNEL_BROADCAST_PORT, NULL,
        CHANNEL_BROADCAST_PORT, udp_rx_channel_selection_callback);
}

bool is_root_reachable()
{
    return NETSTACK_ROUTING.node_is_reachable();
}

void send_ack_to_root()
{
    LOG_INFO("Sending ACK to root\n");
    uint8_t ack = 1;
    simple_udp_sendto(&root_ack_conn, &ack, 1, &root_addr);
}

void configure_root_addr()
{
    NETSTACK_ROUTING.get_root_ipaddr(&root_addr);
    LOG_INFO("Sink reachable: ");
    LOG_INFO_6ADDR(&root_addr);
    LOG_INFO_("\n");
}

void send_to_sink(uint8_t *data, size_t len)
{
    #if (LOG_LEVEL == LOG_LEVEL_DBG)
    LOG_DBG("Sending request %"PRIu32" to ", tx_count);
    LOG_DBG_6ADDR(root_addr);
    LOG_DBG_("\n");
    #endif

    simple_udp_sendto(&udp_conn, data, len, &root_addr);
    tx_count++;
}
/* ==================================================================== */