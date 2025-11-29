#include "producer_net.h"

/* -------------------------------------------------------------------- */
#define LOG_MODULE "[Network]"
#define LOG_LEVEL LOG_LEVEL_INFO

/* -------------------------------------------------------------------- */
static struct simple_udp_connection root_ack_conn;
static struct simple_udp_connection udp_conn;
static struct simple_udp_connection channel_selection_conn;
static uip_ipaddr_t root_addr;
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
    }
}

/* -------------------------------------------------------------------- */
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

bool root_is_known()
{
    if (NETSTACK_ROUTING.get_root_ipaddr(&root_addr))
    {
        LOG_INFO("Sink reachable: ");
        LOG_INFO_6ADDR(&root_addr);
        LOG_INFO_("\n");
        return true;
    }

    return false;
}

void send_ack_to_root()
{
    simple_udp_sendto(&root_ack_conn, ((uint8_t *) 1), 1, &root_addr);
}

void send_to_sink(uint8_t *data, size_t len)
{
    #if (LOG_LEVEL == LOG_LEVEL_DBG)
    LOG_DBG("Sending request %"PRIu32" to ", tx_count);
    LOG_DBG_6ADDR(root_addr);
    LOG_DBG_("\n");
    #endif

    simple_udp_sendto(&udp_conn, data, len, &root_addr);
}
/* ==================================================================== */