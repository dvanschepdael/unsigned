/**
 * @file vram_writer_internal.h
 * @brief Neo Geo-only helper for batching VRAMMOD register writes.
 */

#ifndef UNSIGNED_SYSTEM_VRAM_WRITER_INTERNAL_H
#define UNSIGNED_SYSTEM_VRAM_WRITER_INTERNAL_H

#include "core/types.h"
#include "system/m68k_hotpath.h"

#include <ngdevkit/registers.h>

/**
 * @brief Sets mod on the neogeo VRAM.
 *
 * @param mod Neo Geo VRAMMOD value cached/written for subsequent VRAM accesses.
 */
void unsigned_neogeo_vram_set_mod(u16 mod);

/** Clear a contiguous SCB3 hardware-sprite range without changing any other sprite plane. */
static inline void unsigned_neogeo_vram_clear_scb3_range(u16 first_sprite, u16 sprite_count) {
    unsigned_neogeo_vram_set_mod(1u);
    *REG_VRAMADDR = (u16)(ADDR_SCB3 + first_sprite);
#if defined(__m68k__)
    unsigned_m68k_vram_fill(REG_VRAMRW, 0u, sprite_count);
#else
    for (u16 column = 0u; column < sprite_count; ++column) {
        *REG_VRAMRW = 0u;
    }
#endif
}

#endif
