#ifndef BITPACK_H
#define BUTPACK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* -------------------------------------------- */

// Simple bit writer: writes LSB-first into bytes
typedef struct {
    uint8_t* buf;
    size_t   cap;       // bytes capacity in buf
    size_t   byte_pos;  // current byte index
    uint8_t  bit_pos;   // [0..7], next bit to fill within current byte
} BitWriter;

// Initialize BitWriter
void bw_init(BitWriter* bw, uint8_t* buf, size_t cap);

// Write nbits (<=32) LSBs of value into BitWriter
bool bw_put_bits(BitWriter* bw, uint32_t value, uint8_t nbits);

// Get total bytes used so far in BitWriter
size_t bw_bytes_used(const BitWriter* bw);

// Simple BitReader matching BitWriter's LSB-first packing
typedef struct {
    const uint8_t* buf;
    size_t   cap;       // bytes available
    size_t   byte_pos;
    uint8_t  bit_pos;   // next bit index (0..7)
} BitReader;

void br_init(BitReader* br, const uint8_t* buf, size_t cap);
bool br_get_bits(BitReader* br, uint32_t* value_out, uint8_t nbits);

// Helper funtion for ZigZag encoding
// ZigZag for 16-bit signed -> 16-bit unsigned.
// Maps: 0 -> 0, -1 -> 1, 1 -> 2, -2 -> 3, ...
uint16_t zigzag16(int16_t v);
// Inverse ZigZag (unsigned 16 -> signed 16)
int16_t inv_zigzag16(uint16_t zz);
// Helper function to compute minimum number of bits to represent an unsigned 16-bit integer
uint8_t bits_required_u16(uint16_t x);
                
/* ============================================ */

#endif