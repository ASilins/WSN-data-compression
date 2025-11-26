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

#define UIP_CONF_BUFFER_SIZE 128
#define QUEUEBUF_CONF_NUM 16

#define CSMA_CONF_MAX_FRAME_RETRIES 7

// Some of these things can be adjusted if needed but what is left uncommented has been working on Cooja sims.
// #define RPL_CONF_DIO_INTERVAL_MIN 8
// #define RPL_CONF_DIO_INTERVAL_DOUBLINGS 6
// #define RPL_CONF_DELAY_BEFORE_LEAVING 5
// #define RPL_CONF_DIS_INTERVAL 2000
// #define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 16

#define NETSTACK_CONF_WITH_IPV6 1
#define NETSTACK_CONF_MAC csma_driver
#define NETSTACK_CONF_RDC nullrdc_driver

/* ========================================================================= */
/* ------------------------------- Algorithm ------------------------------- */

#define BLOCK_SIZE 8
#define BLOCK_D 1 // if univariate
/* Our custom header layout:
** [0]   uint8_t seq
** [1]   uint8_t n_in_block
*/
#define HDR_LEN 2 // Default header lenght for all algorithms

/* ---------- Sprintz Configuration ---------- */
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

#define LEARN_SHIFT 1
#define BIT_WIDTH 16
#endif /* SPRINTZ */

#if NONE

#define NUM_SIZE 16

#define PACKED_PAYLOAD_SIZE ((BLOCK_SIZE * NUM_SIZE + 7) / 8)
#define PACKED_BUF_SIZE (HDR_LEN + PACKED_PAYLOAD_SIZE)

#endif /* NONE */

/* ---------- PLA Configuration ---------- */
#if PLA

/**
 * ============================================================
 * MAX_ERROR_THRESHOLD: Core PLA Parameter (Error Threshold)
 * ============================================================
 * 
 * This parameter controls the trade-off between compression 
 * quality and compression ratio:
 * 
 * | Threshold | Max Error | Compression | Use Case           |
 * |-----------|-----------|-------------|-------------------|
 * | 5         | +/-5      | Lower       | High precision    |
 * | 10        | +/-10     | Medium      | General (default) |
 * | 20        | +/-20     | Higher      | Storage limited   |
 * | 50        | +/-50     | Very high   | Trend analysis    |
 * 
 * Error Guarantee: |original - reconstructed| <= MAX_ERROR_THRESHOLD
 * 
 * For DK1 wind power data (range 0-7500):
 * - threshold=10 -> relative error ~0.13%
 * - threshold=20 -> relative error ~0.27%
 * - threshold=50 -> relative error ~0.67%
 */
#define MAX_ERROR_THRESHOLD 10

/**
 * SLOPE_SCALE: Fixed-point scaling factor for slope
 * 
 * Larger value = higher precision
 * 256 is a good default
 */
#define SLOPE_SCALE 256

#endif /* PLA */

/* ========================================================================= */
/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */
