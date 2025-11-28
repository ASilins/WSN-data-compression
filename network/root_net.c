#include "root_net.h"

/* -------------------------------------------------------------------- */
#define LOG_MODULE "[Network]"
#define LOG_LEVEL LOG_LEVEL_DBG

/* -------------------------------------------------------------------- */
static struct simple_udp_connection root_ack_conn;
static struct simple_udp_connection decode_udp_conn;
static struct simple_udp_connection channel_selection_conn;
static uip_ipaddr_t broadcast_addr;
extern struct process main_process;
/* -------------------------------------------------------------------- */
static void
udp_rx_root_ack_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen)
{
    LOG_INFO("Received ACK from mote\n");
    mote_ready = true;
    process_poll(&main_process);
}

static void
udp_rx_decode_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen)
{
    #if (LOG_LEVEL == LOG_LEVEL_DBG)
    LOG_DBG("Received %u bytes from ", datalen);
    LOG_DBG_6ADDR(sender_addr);
    LOG_DBG_("\n");
    #endif /* (LOG_LEVEL == LOG_LEVEL_DBG) */

    decode(data, datalen);
}

/* -------------------------------------------------------------------- */
void set_channel(uint8_t *ch)
{
    LOG_INFO("Switching channel: %d\n", *ch);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, *ch);
}

void start_dag_root()
{
    // Start DAG root
    LOG_INFO("Starting DAG root\n");
    NETSTACK_ROUTING.root_start();

    simple_udp_register(&root_ack_conn, ROOT_ACK_PORT, NULL,
        ROOT_ACK_PORT, udp_rx_root_ack_callback);
    simple_udp_register(&decode_udp_conn, UDP_SERVER_PORT, NULL,
        UDP_CLIENT_PORT, udp_rx_decode_callback);
    simple_udp_register(&channel_selection_conn, CHANNEL_BROADCAST_PORT, NULL,
        CHANNEL_BROADCAST_PORT, NULL);
}

void channel_switch()
{
    NETSTACK_ROUTING.root_start();
}

void broadcast_channel_config(struct channel_config *config)
{
    #if (LOG_LEVEL == LOG_LEVEL_DBG)
    LOG_DBG("Channel broadcast: %d\n", config->ch);
    #endif /* (LOG_LEVEL == LOG_LEVEL_DBG) */

    uip_create_linklocal_allnodes_mcast(&broadcast_addr);
    simple_udp_sendto(&channel_selection_conn, config, sizeof(*config), &broadcast_addr);
}

uint8_t channel_selection()
{
    LOG_INFO("Running channel selection\n");

    uint8_t best_ch = 11;
    int8_t best_res = 127;

    // Loop over the channels
    for (uint8_t k = 11; k <= 26; k++) {
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, k);

        int32_t sum = 0;
        uint8_t iterations = 10;
        for (uint8_t i = 0; i < iterations; i++) {
            radio_value_t rssi;
            NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &rssi);
            sum += rssi;
            clock_wait(CLOCK_SECOND / 20);
        }

        int8_t res = (int8_t)(sum / iterations);

        if (res < best_res) {
            best_ch = k;
            best_res = res;
        }

        #if (LOG_LEVEL == LOG_LEVEL_DBG)
        LOG_DBG("Channel: %d, RSSI average over %d iterations: %d dBm \n", k, iterations, res);
        #endif /* (LOG_LEVEL == LOG_LEVEL_DBG) */
    }

    LOG_INFO("Best channel: %d\n", best_ch);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, IEEE802154_CONF_DEFAULT_CHANNEL);
    return best_ch;
}
/* ==================================================================== */