#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "sys/log.h"
#include "project-conf.h"

/* Algorithm-specific includes */
#if SPRINTZ
#include "sprintz_encoder.h"
#include "bitpack.h"
#endif

#if PLA
#include "pla_encoder.h"
#endif

/**
 * @brief Main encode function - selects algorithm based on compile config
 */
void encode(const int16_t* data,
    int n_in_block,
    uint8_t *packed_buf,
    size_t packed_buf_capacity,
    size_t *packed_len);

#endif /* ENCODER_H */
