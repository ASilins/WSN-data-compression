#include "udp.h"
#include "fire.h"
#include "decoder.h"
#include "project-conf.h"

#include "sys/log.h"
#include <inttypes.h>

#define LOG_MODULE "[Network]"
#define LOG_LEVEL LOG_LEVEL_DBG

#define BLOCK_SIZE 8
#define BLOCK_D 1 // if univariate

static struct simple_udp_connection udp_conn;
static uip_ipaddr_t *sink_addr;

// FIRE parameters
static uint8_t learnShift = 1; // eta = 1/2
static uint8_t bitWidth = 16;  // 8 or 16

static void
udp_rx_decode_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
    static FIREState fire_state_dec;
    static int32_t accum_dec[BLOCK_D];
    static int16_t deltas_dec[BLOCK_D];
    static int16_t prev_sample[BLOCK_D] = {0};

    // Initialize decoder (re-initialize each time for simplicity)
    FIRE_init(&fire_state_dec, BLOCK_D, learnShift, bitWidth, accum_dec, deltas_dec);

    // Buffer for decoded block
    int16_t decoded_block[BLOCK_SIZE * BLOCK_D];

    bool ok = decodeBlock(&fire_state_dec,
                          data, datalen,
                          BLOCK_D,
                          prev_sample,
                          BLOCK_SIZE,
                          decoded_block,
                          prev_sample);

    if (!ok) {
        LOG_ERR("Decode failed for received block\n");
        return;
    }

    // Print decoded block
    LOG_INFO("Decoded block: ");
    for (int i = 0; i < BLOCK_SIZE * BLOCK_D; i++) {
        LOG_INFO_("%d ", decoded_block[i]);
    }
    LOG_INFO_("\n");
}

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

void init_udp_callback()
{
    LOG_INFO("Setting up UDP callback\n");
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
        UDP_SERVER_PORT, udp_rx_callback);
}

void init_udp_root_callback()
{
    LOG_INFO("Setting up UDP root callback\n");

    /* Initialize DAG root */
    NETSTACK_ROUTING.root_start();

    simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
        UDP_CLIENT_PORT, udp_rx_decode_callback);
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