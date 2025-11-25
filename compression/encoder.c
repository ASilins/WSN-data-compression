#include "encoder.h"

#define LOG_MODULE "[Encoder]"
#define LOG_LEVEL LOG_LEVEL_DBG

/* ---------- Algorithm definitions --------- */
#if SPRINTZ

static int16_t errors_out[BLOCK_SIZE * BLOCK_D];
static int16_t last_sample[BLOCK_D] = {0};

void encode_sprintz(const int16_t* data,
    int n_in_block,
    u_int8_t *packed_buf,
    size_t packed_buf_capacity,
    size_t *packed_len)
{
    static FIREState fire_state;
    static int32_t accum[BLOCK_D]; // BLOCK_D = number of columns
    static int16_t deltas[BLOCK_D];

    // We add Sprintz specific header here
    packed_buf[0] = (uint8_t)(*last_sample & 0xff);
    packed_buf[1] = (uint8_t)((*last_sample >> 8) & 0xff);

    FIRE_init(&fire_state, BLOCK_D, (uint8_t)LEARN_SHIFT,
        (uint8_t)BIT_WIDTH, accum, deltas);

    encodeBlock(&fire_state, data, n_in_block, BLOCK_D,
        last_sample, errors_out, last_sample);

    bool ok = bitpack_errors_block(errors_out,
        n_in_block,
        BLOCK_D,
        packed_buf + SPRINTZ_HDR_LEN,
        packed_buf_capacity - SPRINTZ_HDR_LEN,
        packed_len);

    if (!ok) {
        LOG_ERR("Bit-pack overflow or error (n=%d)\n", n_in_block);
    }

    *packed_len += 2;

    #if PRINT_RAW_FIRE_ERRORS
    print_raw_fire_errors(n_in_block);
    #endif
}

#if PRINT_RAW_FIRE_ERRORS
void print_raw_fire_errors(int n_in_block)
{
    // Print raw errors of encoding using FIRE forecaster
    for (int r = 0; r < n_in_block * BLOCK_D; ++r) {
        LOG_INFO_("%d ", errors_out[r]);
    }
}
#endif /* PRINT_RAW_FIRE_ERRORS */

#endif /* SPRINTZ */

#if NONE
void encode_none(const int16_t* data,
    int n_in_block,
    u_int8_t *packed_buf,
    size_t packed_buf_capacity,
    size_t *packed_len)
{
    int buf_pos = 0;
    for (int i = 0; i < n_in_block; i++)
    {
        packed_buf[buf_pos] = (uint8_t)(data[i] & 0xff);
        packed_buf[buf_pos+1] = (uint8_t)((data[i] >> 8) & 0xff);
        *packed_len += 2;
        buf_pos += 2;
    }
}

#endif /* NONE */
/* ========================================== */
/* ------------------ Main ------------------ */

void encode(const int16_t* data,
    int n_in_block,
    uint8_t *packed_buf,
    size_t packed_buf_capacity,
    size_t *packed_len)
{
    #if SPRINTZ
    encode_sprintz(data, n_in_block, packed_buf,
        packed_buf_capacity, packed_len);
    return;
    #endif /* SPRINTZ */

    #if NONE
    encode_none(data, n_in_block, packed_buf,
        packed_buf_capacity, packed_len);
    return;
    #endif /* NONE */

    // Default fall back log
    LOG_ERR("No algorithm specified");
}
/* ========================================== */