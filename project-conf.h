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
#define BLOCK_D 1 // if uninvariate

#if SPRINTZ

// Global Fire params
#define LEARN_SHIFT 1
#define BIT_WIDTH 16

#endif /* SPRINTZ */
/* ========================================================================= */
/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */