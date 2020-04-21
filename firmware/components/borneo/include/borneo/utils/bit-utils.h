#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

inline int get_bit_u8(uint8_t value, int bit) { return value & ((uint8_t)1 << bit); }

inline int get_bit_u16(uint16_t value, int bit) { return value & ((uint16_t)1 << bit); }

inline int get_bit_u32(uint32_t value, int bit) { return value & ((uint32_t)1 << bit); }

inline int get_bit_u64(uint64_t value, int bit) { return value & ((uint64_t)1 << bit); }

/**
 * Brian Kerninghan's algorithm to count 1-bits.
 */

inline int count_high_bits(uint64_t v)
{
    int c; // c accumulates the total bits set in v
    for (c = 0; v; c++) {
        v &= v - 1; // clear the least significant bit set
    }
    return c;
}

#ifdef __cplusplus
}
#endif
