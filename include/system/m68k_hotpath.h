/**
 * @file m68k_hotpath.h
 * @brief C interface to native Motorola 68000 hot-path routines.
 */

#ifndef UNSIGNED_SYSTEM_M68K_HOTPATH_H
#define UNSIGNED_SYSTEM_M68K_HOTPATH_H

#include "core/types.h"

#if defined(__m68k__)

/*
 * Keep the assembly ABI deliberately independent from GCC's -mshort setting:
 * every scalar passed across the C/ASM boundary is widened to 32 bits, while
 * pointers are naturally 32 bits on m68k. The public inline wrappers preserve
 * the engine's compact u16/s16 API without embedding assembly strings in C.
 */
void unsigned_m68k_vram_stream_strided_asm(volatile u16 *port, const u16 *source, u32 count, u32 source_stride_words);

void unsigned_m68k_vram_fill_nonzero_asm(volatile u16 *port, u32 value, u32 count);

void unsigned_m68k_vram_stream_arithmetic_asm(volatile u16 *port, u32 value, u32 count, s32 step);

void unsigned_m68k_vram_stream_tile_attributes_asm(volatile u16 *port, u32 tile, u32 attributes, u32 count, s32 tile_step);

void unsigned_m68k_vram_fill_pair_asm(volatile u16 *port, u32 first, u32 second, u32 count);

/**
 * Stream `count` 16-bit values from a strided source to the Neo Geo VRAM data port.
 *
 * VRAM address/modulo setup remains in the backend C code so hardware policy does
 * not leak into assembly. `source_stride_words` is the distance, in 16-bit words,
 * between source values.
 *
 * @pre `port` and `source` are valid for `count` 16-bit transfers.
 */
static inline void unsigned_m68k_vram_stream_strided(volatile u16 *port, const u16 *source, u16 count, u16 source_stride_words) {
    if (count == 0u) {
        return;
    }

    unsigned_m68k_vram_stream_strided_asm(port, source, (u32)count, (u32)source_stride_words);
}

/** Fill the Neo Geo VRAM data port with the same 16-bit value `count` times. @pre `count > 0`. */
static inline void unsigned_m68k_vram_fill_nonzero(volatile u16 *port, u16 value, u16 count) {
    unsigned_m68k_vram_fill_nonzero_asm(port, (u32)value, (u32)count);
}

/** Checked fill for ranges whose count is a legitimate runtime-zero state. */
static inline void unsigned_m68k_vram_fill(volatile u16 *port, u16 value, u16 count) {
    if (count != 0u) {
        unsigned_m68k_vram_fill_nonzero_asm(port, (u32)value, (u32)count);
    }
}

/**
 * Stream an arithmetic 16-bit sequence to the Neo Geo VRAM data port.
 *
 * The first transfer writes `value`; each following transfer adds `step` to
 * the previous value. A negative step uses normal signed 16-bit two's-complement
 * addition on the 68000.
 *
 * @pre `count > 0`.
 */
static inline void unsigned_m68k_vram_stream_arithmetic(volatile u16 *port, u16 value, u16 count, s16 step) {
    unsigned_m68k_vram_stream_arithmetic_asm(port, (u32)value, (u32)count, (s32)step);
}

/** Stream `tile, attributes` pairs while advancing the tile by `tile_step`. @pre `count > 0`. */
static inline void unsigned_m68k_vram_stream_tile_attributes(volatile u16 *port, u16 tile, u16 attributes, u16 count, s16 tile_step) {
    unsigned_m68k_vram_stream_tile_attributes_asm(port, (u32)tile, (u32)attributes, (u32)count, (s32)tile_step);
}

/** Stream the same two 16-bit words as a pair `count` times. @pre `count > 0`. */
static inline void unsigned_m68k_vram_fill_pair(volatile u16 *port, u16 first, u16 second, u16 count) {
    unsigned_m68k_vram_fill_pair_asm(port, (u32)first, (u32)second, (u32)count);
}

#endif

#endif
