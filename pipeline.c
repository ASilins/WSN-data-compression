#include "contiki.h"

#include "dev/button-sensor.h"

#include "sys/log.h"
#include "pipeline.h"
#include "fire.h"
#include "encoder.h"
#include "decoder.h"
#include "timeseries_data.h"
#include "udp.h"

#define LOG_MODULE "[Pipeline]"
#define LOG_LEVEL LOG_LEVEL_INFO

#define BLOCK_SIZE 8
#define BLOCK_D 1 // if univariate

static int16_t errors_out[BLOCK_SIZE * BLOCK_D];
static int16_t last_sample[BLOCK_D];

PROCESS(pipeline_process, "Pipeline process");

PROCESS_THREAD(pipeline_process, ev, data)
{
    static struct etimer timer;

    // FIRE parameters
    static uint8_t learnShift = 1; // eta = 1/2
    static uint8_t bitWidth = 16;  // 8 or 16

    static FIREState fire_state;
    static int32_t accum[BLOCK_D]; // BLOCK_D = number of columns
    static int16_t deltas[BLOCK_D];
    FIRE_init(&fire_state, BLOCK_D, learnShift, bitWidth, accum, deltas);

    /* ----------- Thread ------------ */
    PROCESS_BEGIN();
    LOG_INFO("Pipeline thread initialised\n");

    SENSORS_ACTIVATE(button_sensor);

    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    etimer_set(&timer, CLOCK_SECOND * 0.2);

    // Buffer for packed output per block:
    // Worst-case payload = BLOCK_SIZE*BLOCK_D*16 bits = 16 bytes + 2-byte header
    // Adjust if BLOCK_SIZE or BLOCK_D changes.
    enum { MAX_W = 16 };
    static uint8_t packed_buf[2 + ((BLOCK_SIZE * BLOCK_D * MAX_W + 7) / 8)];

    static int i = 0;
    for (; i < timeseries_length; i += BLOCK_SIZE) {
        /* --- Yield --- */
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));


        int n_in_block = (i + BLOCK_SIZE <= timeseries_length) ? BLOCK_SIZE : (timeseries_length - i);
        encodeBlock(&fire_state, &timeseries_data[i], n_in_block, BLOCK_D, last_sample, errors_out, last_sample);

        // Pack this block of residuals
        size_t packed_len = 0;
        bool ok = bitpack_errors_block(errors_out, n_in_block, BLOCK_D,
                                       packed_buf, sizeof(packed_buf), &packed_len);

        if (!ok) {
            LOG_ERR_("Bit-pack overflow or error (n=%d)\n", n_in_block);
        } else {
            #if (LOG_LEVEL == LOG_LEVEL_DBG)
            // Print the packed bytes (header + payload) in hex for inspection
            LOG_DBG("PKT: ");
            for (size_t b = 0; b < packed_len; ++b) {
                LOG_DBG_("%02x ", packed_buf[b]);
            }
            LOG_DBG_("\n");
            #endif

            /* ---------- Send data ----------- */
            send_to_sink(packed_buf, packed_len);

            // -------------------------------------------------
            #if ENCODER_SELF_TEST
            // (in the final implementation decoding will be done on sink side)
            static int16_t decoded_block[BLOCK_SIZE * BLOCK_D];
            // Prepare decoder FIRE state copy (must start with SAME predictor state as at encoder start of this block)
            static FIREState fire_state_dec;
            static int32_t accum_dec[BLOCK_D];
            static int16_t deltas_dec[BLOCK_D];
            FIRE_init(&fire_state_dec, BLOCK_D, learnShift, bitWidth, accum_dec, deltas_dec);

            // Reconstruct predictor start (we replay from stream start)
            // For correctness in this simple test, we re-run encoding predictor up to block start.
            // (Inefficient but acceptable for verification; later keep decoder state incrementally.)
            int16_t prev_sample_replay[BLOCK_D] = {0};
            int block_start = i;
            // Replay previous blocks to sync decoder state (ONLY needed because we reinit each time)
            for (int k = 0; k < block_start; k += BLOCK_SIZE) {
                int n_prev = (k + BLOCK_SIZE <= timeseries_length) ? BLOCK_SIZE : (timeseries_length - k);
                encodeBlock(&fire_state_dec, &timeseries_data[k], n_prev, BLOCK_D,
                            prev_sample_replay, errors_out, prev_sample_replay);
            }
            bool dok = decodeBlock(&fire_state_dec,
                                   packed_buf, packed_len,
                                   BLOCK_D,
                                   prev_sample_replay,
                                   BLOCK_SIZE,
                                   decoded_block,
                                   prev_sample_replay);

            if (!dok) {
                LOG_ERR("Decode failed for block starting %d\n", i);
            } else {
                // Compare original vs decoded
                int mismatch = 0;
                for (int r = 0; r < n_in_block; ++r) {
                    int16_t orig = timeseries_data[i + r];
                    int16_t dec  = decoded_block[r];
                    if (orig != dec) {
                        mismatch = 1;
                        LOG_ERR("Mismatch at sample %d: orig=%d dec=%d\n", i + r, orig, dec);
                        break;
                    }
                }
                if (!mismatch) {
                    LOG_INFO("Block %d OK (n=%d)\n", i / BLOCK_SIZE, n_in_block);
                }
            }
            #endif /* ENCODER_SELF_TEST */
            // -------------------------------------------------
        }

        #ifndef PRINT_RAW_FIRE_ERRORS
        // Print raw errors of encoding using FIRE forecaster
        for (int r = 0; r < n_in_block * BLOCK_D; ++r) {
            LOG_INFO_("%d ", errors_out[r]);
        }
        #endif

        /* -- Timer reset --*/
        etimer_reset(&timer);
    }

    LOG_INFO_("\n");
    LOG_INFO("Pipeline finished\n");

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