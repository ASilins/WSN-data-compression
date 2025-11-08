#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

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

// Helper funtion for ZigZag encoding
// ZigZag for 16-bit signed -> 16-bit unsigned.
// Maps: 0 -> 0, -1 -> 1, 1 -> 2, -2 -> 3, ...
static inline uint16_t zigzag16(int16_t v) {
    // Do math in 32-bit, then cast down to avoid UB on shifts
    uint32_t x = (uint32_t)((int32_t)v);
    return (uint16_t)(((uint32_t)(x << 1)) ^ (uint32_t)((int32_t)v >> 15));
}

// Helper function to compute minimum number of bits to represent an unsigned 16-bit integer
static inline uint8_t bits_required_u16(uint16_t x) {
    if (!x) return 0;
    uint8_t n = 0;
    while (x) {
        ++n;
        x >>= 1;
    }
    return n; // 1..16
}

// Simple bit writer: writes LSB-first into bytes
typedef struct {
    uint8_t* buf;
    size_t   cap;       // bytes capacity in buf
    size_t   byte_pos;  // current byte index
    uint8_t  bit_pos;   // [0..7], next bit to fill within current byte
} BitWriter;

// Initialize BitWriter
static inline void bw_init(BitWriter* bw, uint8_t* buf, size_t cap) {
    bw->buf = buf;
    bw->cap = cap;
    bw->byte_pos = 0;
    bw->bit_pos = 0;
    if (cap > 0) {
        bw->buf[0] = 0;
    }
}

// Write nbits (<=32) LSBs of value into BitWriter
static inline bool bw_put_bits(BitWriter* bw, uint32_t value, uint8_t nbits) {
    // Write 'nbits' LSBs of 'value' into the stream, LSB-first
    for (uint8_t i = 0; i < nbits; ++i) {
        if (bw->byte_pos >= bw->cap) {
            return false; // overflow
        }
        uint8_t bit = (uint8_t)((value >> i) & 1u);
        bw->buf[bw->byte_pos] |= (uint8_t)(bit << bw->bit_pos);
        bw->bit_pos++;
        if (bw->bit_pos == 8) {
            bw->bit_pos = 0;
            bw->byte_pos++;
            if (bw->byte_pos < bw->cap) {
                bw->buf[bw->byte_pos] = 0;
            }
        }
    }
    return true;
}

// Get total bytes used so far in BitWriter
static inline size_t bw_bytes_used(const BitWriter* bw) {
    return bw->byte_pos + (bw->bit_pos ? 1u : 0u);
}

// Bit-pack one block of errors:
// Format:
//   header[0] = bw (0..16). If 0 => all zeros, no payload.
//   header[1] = n_in_block (1..BLOCK_SIZE)
//   payload   = n_in_block * D values, each bw bits, LSB-first
//
// out must have at least 2 + ceil(n_in_block*D*bw/8) bytes.
// Returns true on success, false on overflow.
static bool bitpack_errors_block(const int16_t* errors,
                                 int n_in_block,
                                 int D,
                                 uint8_t* out,
                                 size_t out_cap,
                                 size_t* out_len)
{
    // 1) Find max ZigZag value in block
    uint16_t max_zz = 0;
    const int count = n_in_block * D;
    for (int i = 0; i < count; ++i) {
        uint16_t zz = zigzag16(errors[i]);
        if (zz > max_zz) max_zz = zz;
    }

    // 2) Compute bitwidth
    uint8_t bw = bits_required_u16(max_zz); // 0..16

    // 3) Header needs 2 bytes
    if (out_cap < 2) return false;
    out[0] = bw;
    out[1] = (uint8_t)n_in_block;

    // 4) If all zeros, done
    if (bw == 0) {
        if (out_len) *out_len = 2;
        return true;
    }

    // 5) Compute payload size and pack
    const uint32_t total_bits = (uint32_t)count * (uint32_t)bw;
    const size_t payload_bytes = (size_t)((total_bits + 7u) >> 3); // ceil(bits/8)

    if (2 + payload_bytes > out_cap) return false;

    BitWriter bwriter;
    bw_init(&bwriter, out + 2, payload_bytes);

    for (int i = 0; i < count; ++i) {
        uint16_t zz = zigzag16(errors[i]);
        if (!bw_put_bits(&bwriter, (uint32_t)zz, bw)) {
            return false;
        }
    }
    // Sanity: bytes used must equal computed payload_bytes
    const size_t used = bw_bytes_used(&bwriter);
    if (used != payload_bytes) {
        // Shouldn't happen, but keep a guard
        return false;
    }

    if (out_len) *out_len = 2 + payload_bytes;
    return true;
}

void delta_encoding(const int16_t* timeseries_data, int16_t* deltas, unsigned int length) {
    deltas[0] = timeseries_data[0];

    for(unsigned int i = 1; i < length; i++) {
        deltas[i] = timeseries_data[i] - timeseries_data[i - 1];
    }
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

    LOG_INFO_("FIRE initialized with D=%d, learnShift=%d, bitWidth=%d\n",
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


// Tiny helper func used in FIRE_train() - returns -1, 0, +1 for a signed 16-bit input
static inline int32_t signOfNumber(int16_t v) {
    if (v > 0) return 1;
    if (v < 0) return -1;
    return 0;
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

    // Buffer for packed output per block:
    // Worst-case payload = BLOCK_SIZE*BLOCK_D*16 bits = 16 bytes + 2-byte header
    // Adjust if BLOCK_SIZE or BLOCK_D changes.
    enum { MAX_W = 16 };
    static uint8_t packed_buf[2 + ((BLOCK_SIZE * BLOCK_D * MAX_W + 7) / 8)];

    for (int i = 0; i < timeseries_length; i += BLOCK_SIZE) {
        int n_in_block = (i + BLOCK_SIZE <= timeseries_length) ? BLOCK_SIZE : (timeseries_length - i);
        encodeBlock(&fire_state, &timeseries_data[i], n_in_block, BLOCK_D, last_sample, errors_out, last_sample);

        // Pack this block of residuals
        size_t packed_len = 0;
        bool ok = bitpack_errors_block(errors_out, n_in_block, BLOCK_D,
                                       packed_buf, sizeof(packed_buf), &packed_len);

        if (!ok) {
            LOG_ERR_("Bit-pack overflow or error (n=%d)\n", n_in_block);
        } else {
            // Print the packed bytes (header + payload) in hex for inspection
            LOG_INFO_("PKT: ");
            for (size_t b = 0; b < packed_len; ++b) {
                LOG_INFO_("%02x ", packed_buf[b]);
            }
            LOG_INFO_("\n");
            
        }

        /*
        // Print raw errors of encoding using FIRE forecaster
        for (int r = 0; r < n_in_block * BLOCK_D; ++r) {
            LOG_INFO_("%d ", errors_out[r]);
        }*/
    }
    LOG_INFO_("\n");
    LOG_INFO_("Fire prediction + bit packing DONE.\n");

    // TODO: Simulate transmission
    // For TelosB, do not store all errors for the entire dataset (too big):
    // Process block-by-block and transmit or store compressed output immediately.
    /* Sink-side notes:
    The sink must know D and run the same FIRE predictor update 
    (same learnShift, bitWidth) to reconstruct x = pred + err in the same order.
    The first block relies on a shared initial prev_sample 
    (current code uses zeros by default). 
    If we want independent decoding, 
    send an initial raw sample or a reset marker.
    */

    // Free allocated memory

    PROCESS_END();
}