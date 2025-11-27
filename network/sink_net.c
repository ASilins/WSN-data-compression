#include "sink_net.h"

#define LOG_MODULE "[Network]"
#define LOG_LEVEL LOG_LEVEL_DBG

static struct simple_udp_connection sink_udp_conn;
static struct simple_udp_connection channel_udp_conn;

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
    LOG_INFO("Configuring sink UDP connection\n");

    /* Initialize DAG root */
    NETSTACK_ROUTING.root_start();

    simple_udp_register(&sink_udp_conn, UDP_SERVER_PORT, NULL,
        UDP_CLIENT_PORT, udp_rx_sink_decode_callback);

    
    simple_udp_register(&channel_udp_conn, CHANNEL_BROADCAST_PORT, NULL,
        CHANNEL_BROADCAST_PORT, NULL);
}

static uint8_t channel_selection()
{
    LOG_DBG("Starting RSSI Scan...\n");

    uint8_t best_channel = 11;
    int best_result = 127;

    // Loop over the channels
    for (uint8_t k = 11; k <= 26; k++) {
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, k);

        int32_t sum = 0;
        int16_t iterations = 10;
        for (int i = 0; i < iterations; i++) {
            radio_value_t rssi;
            NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &rssi);
            sum += rssi;
            clock_wait(CLOCK_SECOND / 20);
        }

        int result = sum / iterations;

        if (result < best_result) {
            best_channel = k;
            best_result = result;
        }

        LOG_DBG("Channel: %d, RSSI average over %d iterations: %d dBm \n", k, iterations, result);
    }

    LOG_INFO("Found best channel: %d\n", best_channel);

    return best_channel;
}

PROCESS(channel_selection_process, "Channel selection process");

PROCESS_THREAD(channel_selection_process, ev, data)
{
    PROCESS_BEGIN();
    static struct etimer timer;
    static int i = 0;
    static uip_ipaddr_t addr;

    LOG_INFO("Waiting 10 seconds before starting channel selection\n");

    etimer_set(&timer, CLOCK_SECOND * 10);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    LOG_INFO("Running channel selection\n");

    static uint8_t channel;
    channel = channel_selection();

    // Transmit on default channel
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, 26);
    clock_wait(CLOCK_SECOND / 20);

    uip_create_linklocal_allnodes_mcast(&addr);

    for (; i < 4; i++) {
        etimer_set(&timer, CLOCK_SECOND * 2);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
        LOG_DBG("Broadcasting channel %d (attempt %d)\n", channel, i);
        simple_udp_sendto(&channel_udp_conn, &channel, 1, &addr);
    }

    etimer_set(&timer, CLOCK_SECOND * 1);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    LOG_INFO("Switching to channel: %d", channel);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel);

    rpl_global_repair("channel switch");

    PROCESS_END();
}