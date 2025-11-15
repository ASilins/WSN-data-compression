#include "bitpack.h"

// Initialize BitWriter
void bw_init(BitWriter* bw, uint8_t* buf, size_t cap) {
    bw->buf = buf;
    bw->cap = cap;
    bw->byte_pos = 0;
    bw->bit_pos = 0;
    if (cap > 0) {
        bw->buf[0] = 0;
    }
}

// Write nbits (<=32) LSBs of value into BitWriter
bool bw_put_bits(BitWriter* bw, uint32_t value, uint8_t nbits) {
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
size_t bw_bytes_used(const BitWriter* bw) {
    return bw->byte_pos + (bw->bit_pos ? 1u : 0u);
}

// DECODER LOGIC
void br_init(BitReader* br, const uint8_t* buf, size_t cap) {
    br->buf = buf;
    br->cap = cap;
    br->byte_pos = 0;
    br->bit_pos = 0;
}

bool br_get_bits(BitReader* br, uint32_t* value_out, uint8_t nbits) {
    uint32_t v = 0;
    for (uint8_t i = 0; i < nbits; ++i) {
        if (br->byte_pos >= br->cap) {
            return false; // overflow / truncated payload
        }
        uint8_t bit = (uint8_t)((br->buf[br->byte_pos] >> br->bit_pos) & 1u);
        v |= ((uint32_t)bit << i);
        br->bit_pos++;
        if (br->bit_pos == 8) {
            br->bit_pos = 0;
            br->byte_pos++;
        }
    }
    *value_out = v;
    return true;
}

// Helper funtion for ZigZag encoding
// ZigZag for 16-bit signed -> 16-bit unsigned.
// Maps: 0 -> 0, -1 -> 1, 1 -> 2, -2 -> 3, ...
uint16_t zigzag16(int16_t v) {
    // Do math in 32-bit, then cast down to avoid UB on shifts
    uint32_t x = (uint32_t)((int32_t)v);
    return (uint16_t)(((uint32_t)(x << 1)) ^ (uint32_t)((int32_t)v >> 15));
}

// Inverse ZigZag (unsigned 16 -> signed 16)
int16_t inv_zigzag16(uint16_t zz) {
    // (zz >> 1) gives magnitude; lowest bit gives sign
    return (int16_t)((int16_t)(zz >> 1) ^ (int16_t)-(int16_t)(zz & 1));
}

// Helper function to compute minimum number of bits to represent an unsigned 16-bit integer
uint8_t bits_required_u16(uint16_t x) {
    if (!x) return 0;
    uint8_t n = 0;
    while (x) {
        ++n;
        x >>= 1;
    }
    return n; // 1..16
}