#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_
/*---------------------------------------------------------------------------*/
/* ------------------------------- Experiment ------------------------------ */
#define ENCODER_SELF_TEST 0
#define PRINT_RAW_FIRE_ERRORS 0

/* ========================================================================= */
/* -------------------------------- Network -------------------------------- */
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define UIP_CONF_BUFFER_SIZE 128
#define QUEUEBUF_CONF_NUM 16

#define PRODUCER_CALLBACK 0

#define IEEE802154_CONF_DEFAULT_CHANNEL 26

#define CSMA_CONF_MAX_FRAME_RETRIES 7

// Keep radio always on (best lab reliability; higher energy use)
#define NETSTACK_CONF_RDC nullrdc_driver

/* ========================================================================= */
/* ------------------------------- Algorithm ------------------------------- */

#define BLOCK_SIZE 8
#define BLOCK_D 1 // if univariate

/* ---------- Sprintz Configuration ---------- */
#if SPRINTZ

#define LEARN_SHIFT 1
#define BIT_WIDTH 16

#endif /* SPRINTZ */

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
