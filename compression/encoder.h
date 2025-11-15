#ifndef ENCODER_H
#define ENCODER_H

#include "project-conf.h"

#include <stdint.h>

#include "sys/log.h"

// Imports for Sprintz algorithm
#if SPRINTZ
#include "fire.h"
#include "sprintz_encoder.h"
#include "sprintz_decoder.h"

#endif /* SPRINTZ */
/* -------------------------------------------- */

void encode(const int16_t* data,
    int n_in_block,
    uint8_t *packed_buf,
    size_t packed_buf_capacity,
    size_t *packed_len);

/* ============================================ */

#endif /* ENCODER_H */