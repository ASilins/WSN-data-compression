#include "sprintz_encoder.h"
#include "bitpack.h"

/* Bit-pack one block of errors:
** Format:
**   header[0] = bw (0..16). If 0 => all zeros, no payload.
**   header[1] = n_in_block (1..BLOCK_SIZE)
**   payload   = n_in_block * D values, each bw bits, LSB-first
**
** out must have at least 2 + ceil(n_in_block*D*bw/8) bytes.
** Returns true on success, false on overflow. */
bool bitpack_errors_block(const int16_t* errors,
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

        if (zz > max_zz) 
            max_zz = zz;
    }

    // 2) Compute bitwidth
    uint8_t bw = bits_required_u16(max_zz); // 0..16

    // 3) Header needs 2 bytes
    if (out_cap < 2) 
        return false;
        
    out[0] = bw;
    out[1] = (uint8_t)n_in_block;

    // 4) If all zeros, done
    if (bw == 0) {
        if (out_len) 
            *out_len = 2;

        return true;
    }

    // 5) Compute payload size and pack
    const uint32_t total_bits = (uint32_t)count * (uint32_t)bw;
    const size_t payload_bytes = (size_t)((total_bits + 7u) >> 3); // ceil(bits/8)

    if (2 + payload_bytes > out_cap) 
        return false;

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
        return false; // Shouldn't happen, but just to be safe
    }

    if (out_len) 
        *out_len = 2 + payload_bytes;

    return true;
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