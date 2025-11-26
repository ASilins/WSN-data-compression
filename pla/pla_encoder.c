/**
 * @file pla_encoder.c
 * @brief Real PLA Encoder Implementation - Swing Door Algorithm
 * 
 * Swing Door Algorithm Principle:
 * ==============================
 * 
 * Imagine two rays (upper door and lower door) from the anchor point:
 * 
 *     upper door (slope_upper)
 *    /
 *   *-------- anchor point
 *    \
 *     lower door (slope_lower)
 * 
 * When a new point arrives:
 * 1. Calculate slope from anchor to new point
 * 2. If new point is within door range (slope_lower <= slope <= slope_upper):
 *    - Narrow the door range
 *    - Continue current segment
 * 3. If new point exceeds door range:
 *    - End current segment (using middle slope of doors)
 *    - New point becomes anchor for next segment
 * 
 * Error Guarantee:
 * Since door range is +/- threshold, reconstruction error <= threshold
 */

#include "pla_encoder.h"
#include "sys/log.h"

#define LOG_MODULE "PLA_Enc"
#define LOG_LEVEL LOG_LEVEL_DBG

/**
 * Internal segment structure
 */
typedef struct {
    uint8_t start_idx;
    uint8_t end_idx;
    int16_t start_val;
    int16_t slope_q;
} Segment;

/**
 * @brief Swing Door algorithm core implementation
 * 
 * @param samples      Input samples
 * @param n            Number of samples
 * @param threshold    Error threshold
 * @param segments     Output segment array
 * @param max_segs     Maximum number of segments
 * @return int         Actual number of segments
 */
static int swing_door_compress(
    const int16_t *samples,
    int n,
    int16_t threshold,
    Segment *segments,
    int max_segs)
{
    if (n <= 0) return 0;
    if (n == 1) {
        /* Single point: one segment with zero slope */
        segments[0].start_idx = 0;
        segments[0].end_idx = 0;
        segments[0].start_val = samples[0];
        segments[0].slope_q = 0;
        return 1;
    }

    int seg_count = 0;
    int anchor_idx = 0;
    int16_t anchor_val = samples[0];
    
    /* 
     * Door slope range (using fixed-point * SLOPE_SCALE)
     * Initialize to extreme values
     */
    int32_t slope_upper = 32767 * (int32_t)SLOPE_SCALE;  /* Max positive slope */
    int32_t slope_lower = -32768 * (int32_t)SLOPE_SCALE; /* Max negative slope */
    
    int i = 1;
    while (i < n && seg_count < max_segs) {
        /* Calculate distance from anchor to current point */
        int dt = i - anchor_idx;
        int16_t dy = samples[i] - anchor_val;
        
        /* 
         * Calculate allowed slope range (considering error threshold)
         * New point +/- threshold corresponds to slope range
         * 
         * slope_high = (dy + threshold) / dt * SLOPE_SCALE
         * slope_low  = (dy - threshold) / dt * SLOPE_SCALE
         */
        int32_t slope_high = ((int32_t)(dy + threshold) * SLOPE_SCALE) / dt;
        int32_t slope_low  = ((int32_t)(dy - threshold) * SLOPE_SCALE) / dt;
        
        /* 
         * Narrow the door range
         * new upper = min(current upper, new point's upper)
         * new lower = max(current lower, new point's lower)
         */
        int32_t new_upper = (slope_high < slope_upper) ? slope_high : slope_upper;
        int32_t new_lower = (slope_low > slope_lower) ? slope_low : slope_lower;
        
        if (new_lower <= new_upper) {
            /* Door still open: new point within tolerance */
            slope_upper = new_upper;
            slope_lower = new_lower;
            i++;
        } else {
            /* Door closed: need to end current segment */
            
            /* Use middle slope of previous door as segment slope */
            int32_t final_slope = (slope_upper + slope_lower) / 2;
            
            /* Record current segment (excluding current point) */
            segments[seg_count].start_idx = (uint8_t)anchor_idx;
            segments[seg_count].end_idx = (uint8_t)(i - 1);
            segments[seg_count].start_val = anchor_val;
            
            /* Limit slope range */
            if (final_slope > 32767) final_slope = 32767;
            if (final_slope < -32768) final_slope = -32768;
            segments[seg_count].slope_q = (int16_t)final_slope;
            
            LOG_DBG("Segment %d: [%d-%d] start=%d slope_q=%d\n",
                    seg_count, anchor_idx, i-1, anchor_val, (int)final_slope);
            
            seg_count++;
            
            /* Current point becomes new anchor */
            anchor_idx = i;
            anchor_val = samples[i];
            
            /* Reset door range */
            slope_upper = 32767 * (int32_t)SLOPE_SCALE;
            slope_lower = -32768 * (int32_t)SLOPE_SCALE;
            
            i++;
        }
    }
    
    /* Handle last segment */
    if (seg_count < max_segs) {
        int32_t final_slope = (slope_upper + slope_lower) / 2;
        if (final_slope > 32767) final_slope = 32767;
        if (final_slope < -32768) final_slope = -32768;
        
        segments[seg_count].start_idx = (uint8_t)anchor_idx;
        segments[seg_count].end_idx = (uint8_t)(n - 1);
        segments[seg_count].start_val = anchor_val;
        segments[seg_count].slope_q = (int16_t)final_slope;
        
        LOG_DBG("Segment %d (final): [%d-%d] start=%d slope_q=%d\n",
                seg_count, anchor_idx, n-1, anchor_val, (int)final_slope);
        
        seg_count++;
    }
    
    return seg_count;
}

bool pla_encode_block(
    const int16_t *samples,
    int n_in_block,
    uint8_t *out,
    size_t out_cap,
    size_t *out_len)
{
    if (n_in_block <= 0 || n_in_block > BLOCK_SIZE) {
        LOG_ERR("Invalid n_in_block: %d\n", n_in_block);
        *out_len = 0;
        return false;
    }

    /* Run Swing Door algorithm */
    Segment segments[MAX_SEGMENTS_PER_BLOCK];
    int n_segments = swing_door_compress(
        samples, n_in_block, MAX_ERROR_THRESHOLD,
        segments, MAX_SEGMENTS_PER_BLOCK);
    
    if (n_segments <= 0) {
        LOG_ERR("Compression failed\n");
        *out_len = 0;
        return false;
    }

    /* 
     * Output format:
     * Byte 0: n_segments
     * For each segment (5 bytes each):
     *   Byte 0: end_idx
     *   Byte 1-2: start_val (little-endian)
     *   Byte 3-4: slope_q (little-endian)
     */
    size_t needed = 1 + (size_t)n_segments * 5;
    if (out_cap < needed) {
        LOG_ERR("Buffer too small: need %u, have %u\n",
                (unsigned)needed, (unsigned)out_cap);
        *out_len = 0;
        return false;
    }

    /* Write segment count */
    out[0] = (uint8_t)n_segments;
    
    /* Write each segment */
    size_t pos = 1;
    for (int i = 0; i < n_segments; i++) {
        out[pos++] = segments[i].end_idx;
        out[pos++] = (uint8_t)(segments[i].start_val & 0xFF);
        out[pos++] = (uint8_t)((segments[i].start_val >> 8) & 0xFF);
        out[pos++] = (uint8_t)(segments[i].slope_q & 0xFF);
        out[pos++] = (uint8_t)((segments[i].slope_q >> 8) & 0xFF);
    }
    
    *out_len = pos;

    /* Calculate compression ratio */
    // int orig_bytes = n_in_block * 2;
    LOG_INFO("Encoded: n=%d -> %d segments, %u bytes\n",
             n_in_block, n_segments, (unsigned)pos);
    LOG_INFO("Threshold=%d, SLOPE_SCALE=%d\n", 
             MAX_ERROR_THRESHOLD, SLOPE_SCALE);

    return true;
}
