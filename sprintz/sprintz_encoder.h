#ifndef SPRINTZ_ENCODER_H
#define SPRINTZ_ENCODER_H

#include "project-conf.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "fire.h"

/* -------------------------------------------- */

bool bitpack_errors_block(const int16_t* errors,
    int n_in_block, int D, uint8_t* out, size_t out_cap, size_t* out_len);

void encodeBlock(FIREState* s, const int16_t* samples, int n_in_block,
    int D, const int16_t* prev_sample, int16_t* errors_out, int16_t* last_sample);

/* ============================================ */
#endif /* SPRINTZ_ENCODER_H */