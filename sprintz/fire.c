#include "fire.h"
#include "sys/log.h"

#define LOG_MODULE "[Fire]"
#define LOG_LEVEL LOG_LEVEL_INFO

// Tiny helper func used in FIRE_train() - returns -1, 0, +1 for a signed 16-bit input
static inline int32_t signOfNumber(int16_t v) {
    if (v > 0) return 1;
    if (v < 0) return -1;
    return 0;
}

// For initializing FIREState
void FIRE_init(FIREState* state, int D, uint8_t learnShift, uint8_t w, int32_t* accum, int16_t* deltas) {
    if (D <= 0) {
        LOG_ERR_("FIRE_init: D must be positive\n");
        return;
    }
    if (w != 8 && w != 16) {
        LOG_ERR_("FIRE_init: bitWidth must be 8 or 16\n");
        return;
    }
    if (accum == NULL || deltas == NULL) {
        LOG_ERR_("FIRE_init: accum and deltas must be non-NULL\n");
        return;
    }
    // Optional: keep η reasonable. learnShift = 1 -> η = 1/2 (paper default).
    if (learnShift > 7) {
        LOG_ERR_("FIRE_init: learnShift too large (got %u). Try 0..7.\n", (unsigned)learnShift);
        return;
    }

    state->D = D;
    state->learnShift = learnShift;  //
    state->bitWidth = w;
    state->accum = accum; // for w==16, this must be int32_t[D]
    state->deltas = deltas; // int16_t[D]

    // Zero-initialize state
    for (int i = 0; i < D; i++) {
        state->accum[i]  = 0;
        state->deltas[i] = 0;
    }

    LOG_DBG("FIRE initialized with D=%d, learnShift=%d, bitWidth=%d\n",
              D, (int)learnShift, (int)w);
}

// For predicting next sample
void FIRE_predict(FIREState* s, const int16_t* prev_sample, int16_t* out_pred) {
    const int D= s->D;
    const int w = s->bitWidth; // 8 or 16
    const int learnShift = s->learnShift;  // eta = 2^-learnShift

    // Precompute clamp bounds in 32-bit
    const int32_t x_min = -(1L << (w - 1));
    const int32_t x_max =  (1L << (w - 1)) - 1;

    for (int i = 0; i < D; ++i) {
        // alpha
        int32_t alpha = (int32_t)(s->accum[i]) >> learnShift;

        // Predict next delta: delta_hat = (alpha * prev_delta) >> w
        int32_t prev_delta = (int32_t)s->deltas[i];

        // Use a wide enough product to avoid overflow before >> w
        int32_t delta_hat;
        if (w == 16) {
            int64_t wide_prod = (int64_t)alpha * (int64_t)prev_delta; // 64-bit
            delta_hat = (int32_t)(wide_prod >> 16); // back to 16-bit scale
        } else { // w == 8
            int32_t wide_prod = (int32_t)alpha * (int32_t)prev_delta; // 32-bit is enough
            delta_hat = (int32_t)(wide_prod >> 8); // back to 8-bit scale
        }

        // Predict next sample in 32-bit to avoid overflow
        int32_t x_prev = (int32_t)prev_sample[i];
        int32_t x_hat  = x_prev + delta_hat;

        // Clamp to the signed w-bit range
        if (x_hat < x_min) x_hat = x_min;
        if (x_hat > x_max) x_hat = x_max;

        // Store as int16_t
        out_pred[i] = (int16_t)x_hat;
    }
}

// For training/updating FIRE state
void FIRE_train(FIREState* s, const int16_t* prev_sample, const int16_t* x, const int16_t* err)
{
    const int D = s->D;

    for (int i = 0; i < D; ++i) {
        // Gradient: grad = -sign(err[i]) * δ_{i-1}
        int32_t prev_delta = (int32_t)s->deltas[i]; // δ_{i-1}
        int32_t sgn = signOfNumber(err[i]); // -1, 0, +1
        int32_t grad = -(sgn) * prev_delta;

        // Update accumulator (scaled α)
        s->accum[i] -= grad;

        // Update stored delta: δ_i = x[i] - x[i-1]
        int32_t true_delta = (int32_t)x[i] - (int32_t)prev_sample[i];
        s->deltas[i] = (int16_t)true_delta;
    }
}