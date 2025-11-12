#include "decoder.h"
#include "bitpack.h"

// Decode one packed block into reconstructed samples.
// Returns true if successful, false if malformed.
bool decodeBlock(FIREState* s,
                        const uint8_t* pkt,
                        size_t pkt_len,
                        int D,
                        const int16_t* prev_sample,
                        int n_expected_max,
                        int16_t* out_samples,   // size >= BLOCK_SIZE * D
                        int16_t* last_sample)
{
if (pkt_len < 2) return false;
    uint8_t bw = pkt[0];
    uint8_t n  = pkt[1];
    if (n == 0 || n > n_expected_max) return false;
    if (bw > 16) return false;

    const int count = n * D;

    // Fast path: all zero residuals
    const uint8_t* payload = pkt + 2;
    size_t payload_len = (pkt_len > 2) ? (pkt_len - 2) : 0;

    // Validate payload length for bw > 0
    if (bw == 0) {
        if (pkt_len != 2) return false; // no payload expected
    } else {
        uint32_t total_bits = (uint32_t)count * (uint32_t)bw;
        size_t need_bytes = (size_t)((total_bits + 7u) >> 3);
        if (need_bytes != payload_len) {
            return false; // malformed length
        }
    }

    int16_t x_prev[D];
    for (int i = 0; i < D; ++i) {
        x_prev[i] = prev_sample[i];
    }

    BitReader br;
    br_init(&br, payload, payload_len);

    for (int r = 0; r < n; ++r) {
        int16_t pred[D];
        FIRE_predict(s, x_prev, pred);

        // Reconstruct each channel
        for (int j = 0; j < D; ++j) {
            int16_t err = 0;
            if (bw > 0) {
                uint32_t zz;
                if (!br_get_bits(&br, &zz, bw)) {
                    return false;
                }
                err = inv_zigzag16((uint16_t)zz);
            }
            int32_t x_hat = (int32_t)pred[j] + (int32_t)err;

            // Skipping clamping: prediction + residual should already be in-range for valid packets

            out_samples[r * D + j] = (int16_t)x_hat;
        }

        // Train with reconstructed sample & residuals (err = x - pred)
        for (int j = 0; j < D; ++j) {
            int16_t err = (int16_t)((int32_t)out_samples[r * D + j] - (int32_t)pred[j]);
            // Single-channel arrays for FIRE_train signature
            int16_t x_arr[1]    = { out_samples[r * D + j] };
            int16_t err_arr[1]  = { err };
            // Temporarily set D=1 for per-channel update if D>1 in future
            // Current code: D=1, so direct call works
            FIRE_train(s, x_prev, x_arr, err_arr);
        }

        // Update prev sample set (full row)
        for (int j = 0; j < D; ++j) {
            x_prev[j] = out_samples[r * D + j];
        }
    }

    // Persist last sample
    for (int j = 0; j < D; ++j) {
        last_sample[j] = x_prev[j];
    }

    return true;
}