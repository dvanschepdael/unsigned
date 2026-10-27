/**
 * @file vram_writer.h
 * @brief Neo Geo-only helpers for batching VRAM writes and target hot paths.
 */

#ifndef UNSIGNED_SYSTEM_VRAM_WRITER_H
#define UNSIGNED_SYSTEM_VRAM_WRITER_H

#include "core/types.h"

#include <ngdevkit/registers.h>

/*
 * Low-level VRAM writer implementation boundary. The Neo Geo build links the
 * sibling vram_writer_asm.asm implementation; host renderer tests link a
 * C implementation from test/mock. Scalar arguments are widened to 32 bits so
 * the target C/ASM ABI is independent of -mshort.
 */
void unsigned_system_vram_fill_asm(volatile u16 *port, u32 value, u32 count);
void unsigned_system_vram_fill_pair_asm(volatile u16 *port, u32 first, u32 second, u32 count);
void unsigned_system_vram_stream_arithmetic_asm(volatile u16 *port, u32 value, u32 count, s32 step);
void unsigned_system_vram_stream_strided_asm(volatile u16 *port, const u16 *source, u32 count, u32 source_stride_words);
void unsigned_system_vram_stream_tiles_asm(volatile u16 *port, u32 tile, u32 attributes, u32 count, s32 tile_step);

/** Fill the Neo Geo VRAM data port with the same value. @pre count > 0. */
static inline void unsigned_system_vram_fill(volatile u16 *port, u16 value, u16 count) {
    unsigned_system_vram_fill_asm(port, (u32)value, (u32)count);
}

/** Stream the same two words as a pair count times. @pre count > 0. */
static inline void unsigned_system_vram_fill_pair(volatile u16 *port, u16 first, u16 second, u16 count) {
    unsigned_system_vram_fill_pair_asm(port, (u32)first, (u32)second, (u32)count);
}

/** Stream value, value+step, ... to VRAM. @pre count > 0. */
static inline void unsigned_system_vram_stream_arithmetic(volatile u16 *port, u16 value, u16 count, s16 step) {
    unsigned_system_vram_stream_arithmetic_asm(port, (u32)value, (u32)count, (s32)step);
}

/** Stream count words from a strided source. A zero count is a valid no-op. */
static inline void unsigned_system_vram_stream_strided(volatile u16 *port, const u16 *source, u16 count, u16 source_stride_words) {
    if (count == 0u) {
        return;
    }

    unsigned_system_vram_stream_strided_asm(port, source, (u32)count, (u32)source_stride_words);
}

/** Stream tile,attributes pairs while advancing tile. @pre count > 0. */
static inline void unsigned_system_vram_stream_tiles(volatile u16 *port, u16 tile, u16 attributes, u16 count, s16 tile_step) {
    unsigned_system_vram_stream_tiles_asm(port, (u32)tile, (u32)attributes, (u32)count, (s32)tile_step);
}


/**
 * @brief Sets mod on the neogeo VRAM.
 *
 * @param mod Neo Geo VRAMMOD value cached/written for subsequent VRAM accesses.
 */
void unsigned_system_vram_set_mod(u16 mod);

/** Clear a contiguous SCB3 hardware-sprite range without changing any other sprite plane. */
static inline void unsigned_system_vram_clear_scb3(u16 first_sprite, u16 sprite_count) {
    unsigned_system_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + first_sprite);
    if (sprite_count != 0u) {
        unsigned_system_vram_fill(REG_VRAMRW, 0u, sprite_count);
    }
}

#endif
