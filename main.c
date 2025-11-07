#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#include "contiki.h"
#include "timeseries_data.h"
#include "sys/log.h"

#define LOG_MODULE "Mini-Project - TimeSeries"
#define LOG_LEVEL LOG_LEVEL_INFO

#define BLOCK_SIZE 8
#define BLOCK_D 1 // if univariate

static int16_t errors_out[BLOCK_SIZE * BLOCK_D];
static int16_t last_sample[BLOCK_D];

typedef struct {
    int D;
    uint8_t learnShift;  // e.g., 1 => η=1/2
    uint8_t bitWidth;    // 8 or 16
    int32_t *accum;      // length D
    int16_t *deltas;     // length D
} FIREState;


void delta_encoding(const int16_t* timeseries_data, int16_t* deltas, unsigned int length) {
    deltas[0] = timeseries_data[0];

    for(unsigned int i = 1; i < length; i++) {
        deltas[i] = timeseries_data[i] - timeseries_data[i - 1];
    }
}


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

    LOG_INFO_("FIRE initialized with D=%d, learnShift=%d, bitWidth=%d\n",
              D, (int)learnShift, (int)w);
}

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


// Tiny helper func used in FIRE_train() - returns -1, 0, +1 for a signed 16-bit input
static inline int32_t signOfNumber(int16_t v) {
    if (v > 0) return 1;
    if (v < 0) return -1;
    return 0;
}

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


void encodeBlock(FIREState* s, 
    const int16_t* samples, 
    int n_in_block, 
    int D, 
    const int16_t* prev_sample, 
    int16_t* errors_out, 
    int16_t* last_sample) 
{
    int16_t x_prev[D];

    for (int i = 0; i < D; ++i) {
        x_prev[i] = prev_sample[i];
    }

    int16_t pred[D];

    for (int r = 0; r < n_in_block; ++r) {
        const int16_t* x_row = &samples[r * D];
        int16_t* e_row = &errors_out[r * D];

        FIRE_predict(s, x_prev, pred);

        // err = x - pred (store), then train with (prev, x, err)
        for (int j = 0; j < D; ++j) {
            e_row[j] = (int16_t)((int32_t)x_row[j] - (int32_t)pred[j]);
        }

        FIRE_train(s, x_prev, x_row, e_row);

        for (int j = 0; j < D; ++j) {
            x_prev[j] = x_row[j];
        }
    }

    for (int j = 0; j < D; ++j) {
        last_sample[j] = x_prev[j];
    }

}

PROCESS(main_process, "Main process");
AUTOSTART_PROCESSES(&main_process);

PROCESS_THREAD(main_process, ev, data) {
    PROCESS_BEGIN();

    /*
    // mote runs out of memory for dynamic allocation, so have to use static arrays
    // int16_t* deltas = malloc(timeseries_length * sizeof(int16_t));
    static int16_t deltas[768];
    delta_encoding(timeseries_data, deltas, timeseries_length);
    
    LOG_INFO_("Delta Encoded Data DONE.\n");
    */

    // FIRE parameters
    uint8_t learnShift = 1; // eta = 1/2
    uint8_t bitWidth = 16;  // 8 or 16

    static FIREState fire_state;
    static int32_t accum[BLOCK_D]; // BLOCK_D = number of columns
    static int16_t deltas[BLOCK_D];
    FIRE_init(&fire_state, BLOCK_D, learnShift, bitWidth, accum, deltas);

    for (int i = 0; i < timeseries_length; i += BLOCK_SIZE) {
        int n_in_block = (i + BLOCK_SIZE <= timeseries_length) ? BLOCK_SIZE : (timeseries_length - i);
        encodeBlock(&fire_state, &timeseries_data[i], n_in_block, BLOCK_D, last_sample, errors_out, last_sample);

        // Print only the errors produced in this block
        for (int r = 0; r < n_in_block * BLOCK_D; ++r) {
            LOG_INFO_("%d ", errors_out[r]);
        }
    }
    LOG_INFO_("\n");
    LOG_INFO_("Fire prediction DONE.\n");

    // TODO: Bit-pack errors_out and simulate transmission
    // For TelosB, do not store all errors for the entire dataset (too big):
    // Process block-by-block and transmit or store compressed output immediately.


    // Free allocated memory

    PROCESS_END();
}