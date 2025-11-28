#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_
/*---------------------------------------------------------------------------*/
/* ------------------------------- Experiment ------------------------------ */
#define ENCODER_SELF_TEST 0
#define PRINT_RAW_FIRE_ERRORS 0

/* ========================================================================= */
/* -------------------------------- Network -------------------------------- */
#define ROOT_ACK_PORT 8081
#define UDP_CLIENT_PORT	8080
#define UDP_SERVER_PORT	7070
#define CHANNEL_BROADCAST_PORT 4000

#define IEEE802154_CONF_DEFAULT_CHANNEL 26

#if ROOT
#define CHANNEL_BROADCAST_COUNT 5
#define CHANNEL_BROADCAST_DELAY 1
#endif /* ROOT */

#if PRODUCER
#define RPL_CONF_DELAY_BEFORE_LEAVING 5
#endif /* PRODUCER */

#define UIP_CONF_BUFFER_SIZE 128
#define QUEUEBUF_CONF_NUM 16


#define CSMA_CONF_MAX_FRAME_RETRIES 7

// Keep this for now
// #define RPL_CONF_DIO_INTERVAL_MIN 12
// #define RPL_CONF_DIO_INTERVAL_DOUBLINGS 6

// Keep radio always on (best lab reliability; higher energy use)
#define NETSTACK_CONF_RDC nullrdc_driver

/* ========================================================================= */
/* ------------------------------- Algorithm ------------------------------- */

#define BLOCK_SIZE 8
/* Our custom header layout:
** [0]   uint8_t seq
** [1]   uint8_t n_in_block
*/
#define HDR_LEN 2 // Default header lenght for all algorithms

#if SPRINTZ
/* --- buffer size spec --- */
#define BLOCK_D 1 // if uninvariate

/* Sprintz header additional info:
** [4..5]   int16_t  prev_sample (for BLOCK_D == 1)
*/
#define SPRINTZ_HDR_LEN 2

/* Buffer for packed output per block:
** Worst-case payload:
**      BLOCK_SIZE * BLOCK_D * 16 bits + 6-byte header.
** Computed as:
**      ((BLOCK_SIZE * BLOCK_D * MAX_W + 7) / 8) + HDR_LEN.
*/
#define MAX_W 16

// Defining packed buffer size
#define PACKED_PAYLOAD_SIZE ((BLOCK_SIZE * BLOCK_D * MAX_W + 7) / 8)
#define PACKED_BUF_SIZE (HDR_LEN + SPRINTZ_HDR_LEN + PACKED_PAYLOAD_SIZE)
/* ======================== */

// Global Fire params
#define LEARN_SHIFT 1
#define BIT_WIDTH 16
#endif /* SPRINTZ */

#if NONE

#define NUM_SIZE 16

#define PACKED_PAYLOAD_SIZE ((BLOCK_SIZE * NUM_SIZE + 7) / 8)
#define PACKED_BUF_SIZE (HDR_LEN + PACKED_PAYLOAD_SIZE)

#endif /* NONE */
/* ========================================================================= */
/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */