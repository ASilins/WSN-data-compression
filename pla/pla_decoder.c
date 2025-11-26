/**
 * @file pla_decoder.c
 * @brief Real PLA Decoder Implementation
 * 
 * Reconstructs original time series from multiple segments
 */

#include "pla_decoder.h"
#include "sys/log.h"

#define LOG_MODULE "PLA_Dec"
#define LOG_LEVEL LOG_LEVEL_INFO

bool pla_decode_block(
    const uint8_t *pkt,
    size_t pkt_len,
    int n_expected,
    int16_t *out_samples)
{
    /* Need at least 1 byte for segment count */
    if (pkt_len < 1) {
        LOG_ERR("Packet too short\n");
        return false;
    }

    if (n_expected <= 0 || n_expected > BLOCK_SIZE) {
        LOG_ERR("Invalid n_expected: %d\n", n_expected);
        return false;
    }

    /* Read segment count */
    uint8_t n_segments = pkt[0];
    
    if (n_segments == 0 || n_segments > BLOCK_SIZE) {
        LOG_ERR("Invalid n_segments: %d\n", n_segments);
        return false;
    }

    /* Validate data length */
    size_t expected_len = 1 + (size_t)n_segments * 5;
    if (pkt_len < expected_len) {
        LOG_ERR("Packet too short: need %u, got %u\n",
                (unsigned)expected_len, (unsigned)pkt_len);
        return false;
    }

    LOG_DBG("Decoding: %d segments\n", n_segments);

    /* 
     * Decode each segment and reconstruct data
     * 
     * Segment format (5 bytes each):
     *   Byte 0: end_idx
     *   Byte 1-2: start_val (little-endian)
     *   Byte 3-4: slope_q (little-endian)
     */
    size_t pos = 1;
    int current_start = 0;  /* Current segment start index */
    
    for (int seg = 0; seg < n_segments; seg++) {
        uint8_t end_idx = pkt[pos++];
        int16_t start_val = (int16_t)(pkt[pos] | (pkt[pos + 1] << 8));
        pos += 2;
        int16_t slope_q = (int16_t)(pkt[pos] | (pkt[pos + 1] << 8));
        pos += 2;
        
        LOG_DBG("Segment %d: [%d-%d] start=%d slope_q=%d\n",
                seg, current_start, end_idx, start_val, slope_q);

        /* Validate index */
        if (end_idx >= n_expected || end_idx < current_start) {
            LOG_ERR("Invalid segment indices: start=%d end=%d\n",
                    current_start, end_idx);
            return false;
        }

        /* Reconstruct all points in this segment */
        for (int t = current_start; t <= end_idx && t < n_expected; t++) {
            /* 
             * Reconstruction formula:
             * value[t] = start_val + slope_q * (t - start_idx) / SLOPE_SCALE
             */
            int dt = t - current_start;
            int32_t value = (int32_t)start_val + 
                           ((int32_t)slope_q * dt) / SLOPE_SCALE;
            
            /* Limit range */
            if (value > 32767) value = 32767;
            if (value < -32768) value = -32768;
            
            out_samples[t] = (int16_t)value;
        }

        /* Next segment start */
        current_start = end_idx + 1;
    }

    /* Verify all points are filled */
    if (current_start < n_expected) {
        LOG_WARN("Not all samples reconstructed: got %d, need %d\n",
                 current_start, n_expected);
        /* Fill remaining with last value */
        int16_t last_val = (current_start > 0) ? out_samples[current_start - 1] : 0;
        for (int t = current_start; t < n_expected; t++) {
            out_samples[t] = last_val;
        }
    }

    /* Print reconstructed result */
    LOG_INFO("Reconstructed (n=%d, %d segments): ", n_expected, n_segments);
    for (int i = 0; i < n_expected; i++) {
        LOG_INFO_("%d ", (int)out_samples[i]);
    }
    LOG_INFO_("\n");

    return true;
}
