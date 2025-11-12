#ifndef DECODER_H
#define DECODER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "fire.h"

bool decodeBlock(FIREState* s, const uint8_t* pkt, size_t pkt_len, int D,
                const int16_t* prev_sample, int n_expected_max,
                int16_t* out_samples,   // size >= BLOCK_SIZE * D
                int16_t* last_sample);

#endif