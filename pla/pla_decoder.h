/**
 * @file pla_decoder.h
 * @brief Real PLA Decoder - Reconstruct data from multiple segments
 */

#ifndef PLA_DECODER_H
#define PLA_DECODER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "project-conf.h"

/**
 * Slope fixed-point scaling factor (must match encoder)
 */
#ifndef SLOPE_SCALE
#define SLOPE_SCALE 256
#endif

/**
 * @brief Decode a PLA compressed block
 * 
 * Input format:
 *   Byte 0: n_segments
 *   For each segment:
 *     Byte 0: end_idx
 *     Byte 1-2: start_val (little-endian)
 *     Byte 3-4: slope_q (little-endian)
 * 
 * Reconstruction formula:
 *   For segment from start_idx to end_idx:
 *   value[t] = start_val + (slope_q * (t - start_idx)) / SLOPE_SCALE
 * 
 * @param pkt           Input data (PLA payload)
 * @param pkt_len       Input data length
 * @param n_expected    Expected number of output samples
 * @param out_samples   Output sample array
 * @return true         Decoding successful
 * @return false        Decoding failed
 */
bool pla_decode_block(
    const uint8_t *pkt,
    size_t pkt_len,
    int n_expected,
    int16_t *out_samples
);

#endif /* PLA_DECODER_H */
