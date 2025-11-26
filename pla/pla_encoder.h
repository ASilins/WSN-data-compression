/**
 * @file pla_encoder.h
 * @brief Real Piecewise Linear Approximation (PLA) Encoder
 * 
 * Implements the Swing Door Trending algorithm with error threshold control
 * 
 * Key Features:
 * - max_error_threshold: Controls maximum allowed reconstruction error
 * - Dynamic segment length: Automatically determined based on error
 * - Error guarantee: Reconstruction error <= max_error_threshold
 */

#ifndef PLA_ENCODER_H
#define PLA_ENCODER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "project-conf.h"

/**
 * ============================================================
 * Core Parameter: MAX_ERROR_THRESHOLD
 * ============================================================
 * 
 * This is the core parameter controlling compression quality vs ratio:
 * 
 * | Threshold | Effect              | Use Case              |
 * |-----------|---------------------|----------------------|
 * | 5         | High precision      | Precise monitoring   |
 * | 10        | Medium (recommended)| General applications |
 * | 20        | High compression    | Storage constrained  |
 * | 50        | Very high compress  | Trend analysis       |
 * 
 * Can be overridden in project-conf.h
 */
#ifndef MAX_ERROR_THRESHOLD
#define MAX_ERROR_THRESHOLD 10
#endif

/**
 * Slope fixed-point scaling factor
 */
#ifndef SLOPE_SCALE
#define SLOPE_SCALE 256
#endif

/**
 * Maximum number of segments per block
 */
#define MAX_SEGMENTS_PER_BLOCK BLOCK_SIZE

/**
 * @brief Encode a block using real PLA (Swing Door algorithm)
 * 
 * Algorithm steps:
 * 1. Start from the first point (anchor)
 * 2. For each new point, check if it's within the "door" range
 * 3. If error exceeds threshold, end current segment, start new one
 * 4. Output multiple segment parameters
 * 
 * Output format:
 *   Byte 0: n_segments (number of segments)
 *   For each segment:
 *     Byte 0: end_idx (end index of this segment)
 *     Byte 1-2: start_val (start value, int16_t)
 *     Byte 3-4: slope_q (quantized slope, int16_t)
 * 
 * @param samples       Input sample array
 * @param n_in_block    Number of samples in block (usually BLOCK_SIZE=8)
 * @param out           Output buffer
 * @param out_cap       Output buffer capacity
 * @param out_len       [out] Actual output bytes
 * @return true         Encoding successful
 * @return false        Encoding failed
 */
bool pla_encode_block(
    const int16_t *samples,
    int n_in_block,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len
);

/**
 * @brief Get current error threshold value
 */
static inline int16_t pla_get_threshold(void) {
    return MAX_ERROR_THRESHOLD;
}

#endif /* PLA_ENCODER_H */
