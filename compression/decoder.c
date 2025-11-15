#include "decoder.h"

#define LOG_MODULE "[Decoder]"
#define LOG_LEVEL LOG_LEVEL_INFO

#if SPRINTZ
void decode_using_sprintz(const uint8_t *data, uint16_t datalen)
{
    static FIREState fire_state_dec;
    static int32_t accum_dec[BLOCK_D];
    static int16_t deltas_dec[BLOCK_D];
    static int16_t prev_sample[BLOCK_D] = {0};

    // Initialize decoder (re-initialize each time for simplicity)
    FIRE_init(&fire_state_dec, BLOCK_D, (uint8_t)LEARN_SHIFT, (uint8_t)BIT_WIDTH, accum_dec, deltas_dec);

    // Buffer for decoded block
    int16_t decoded_block[BLOCK_SIZE * BLOCK_D];

    bool ok = decodeBlock(&fire_state_dec,
                          data, datalen,
                          BLOCK_D,
                          prev_sample,
                          BLOCK_SIZE,
                          decoded_block,
                          prev_sample);

    if (!ok) {
        LOG_ERR("Decode failed for received block\n");
        return;
    }

    // Print decoded block
    LOG_INFO("Decoded block: ");
    for (int i = 0; i < BLOCK_SIZE * BLOCK_D; i++) {
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