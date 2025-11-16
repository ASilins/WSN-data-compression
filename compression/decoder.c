#include "decoder.h"

#define LOG_MODULE "[Decoder]"
#define LOG_LEVEL LOG_LEVEL_INFO

#if SPRINTZ
void decode_using_sprintz(const uint8_t *data, uint16_t datalen)
{
        // Header: seq(2) + n_in_block(2) + prev_sample(2)  (BLOCK_D==1)
    enum { HDR_LEN = 2 + 2 + 2 };
    if (datalen < HDR_LEN) {
        LOG_ERR("Packet too short for header\n");
        return;
    }

    uint16_t seq = (uint16_t)(data[0] | (data[1] << 8));
    uint16_t n_in_block = (uint16_t)(data[2] | (data[3] << 8));
    int16_t prev_sample_val = (int16_t)(data[4] | (data[5] << 8));

    const uint8_t *payload = data + HDR_LEN;
    uint16_t payload_len = (uint16_t)(datalen - HDR_LEN);

    LOG_DBG("Decoding seq=%u, n=%u\n", seq, n_in_block);

    static FIREState fire_state_dec;
    static int32_t accum_dec[BLOCK_D];
    static int16_t deltas_dec[BLOCK_D];
    static int16_t prev_sample[BLOCK_D] = {0};

    // Initialize decoder (re-initialize each time for simplicity)
    FIRE_init(&fire_state_dec, BLOCK_D, (uint8_t)LEARN_SHIFT, (uint8_t)BIT_WIDTH, accum_dec, deltas_dec);

    // Set prev_sample from header (for BLOCK_D==1)
    prev_sample[0] = prev_sample_val;

    // Buffer for decoded block
    int16_t decoded_block[BLOCK_SIZE * BLOCK_D];

    bool ok = decodeBlock(&fire_state_dec,
                          payload, payload_len,
                          BLOCK_D,
                          prev_sample,
                          n_in_block,
                          decoded_block,
                          prev_sample);

    if (!ok) {
        LOG_ERR("Decode failed for received block\n");
        return;
    }

    // Print decoded block
    LOG_INFO("Decoded block: ");
    for (int i = 0; i < n_in_block * BLOCK_D; i++) {
        LOG_INFO_("%d ", decoded_block[i]);
    }
    LOG_INFO_("\n");
}
#endif /* SPRINTZ */

void decode(const uint8_t *data, uint16_t datalen)
{
    #if SPRINTZ
    decode_using_sprintz(data, datalen);
    return;
    #endif /* SPRINTZ */

    LOG_ERR("No algorithm specified for decoding");
}