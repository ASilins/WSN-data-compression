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
#define QUEUEBUF_CONF_NUM 4

#define PRODUCER_CALLBACK 0

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