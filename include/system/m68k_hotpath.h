/**
 * @file m68k_hotpath.h
 * @brief Narrow optional 68000 assembly helpers for measured Neo Geo hot paths.
 *
 * The normal build stays C-only. Define UNSIGNED_USE_M68K_VRAM_ASM=1 on the real
 * m68k-neogeo-elf toolchain only after comparing generated code with tools/inspect_m68k_hotspots.sh.
 */

#ifndef UNSIGNED_SYSTEM_M68K_HOTPATH_H
#define UNSIGNED_SYSTEM_M68K_HOTPATH_H

#include "core/types.h"

#ifndef UNSIGNED_USE_M68K_VRAM_ASM
#define UNSIGNED_USE_M68K_VRAM_ASM 0
#endif

#if defined(__m68k__) && UNSIGNED_USE_M68K_VRAM_ASM
#define UNSIGNED_M68K_VRAM_ASM_ACTIVE 1

/**
 * Stream `count` 16-bit values from a strided source to the Neo Geo VRAM data port.
 *
 * This helper intentionally owns only the tight data loop. VRAM address/modulo setup remains in
 * the backend C code so hardware policy does not leak into assembly. `source_stride_words` is the
 * distance, in 16-bit words, between two source values.
 * @pre `port` and `source` are valid for `count` 16-bit transfers.
 */
static inline void unsigned_m68k_vram_stream_strided(volatile u16 *port, const u16 *source, u16 count, u16 source_stride_words) {
    if (count == 0u) {
        return;
    }

    const u16 *cursor = source;
    u16 loops = (u16)(count - 1u);
    const u16 stride_bytes = (u16)(source_stride_words << 1u);

    __asm__ volatile("1:\n\t"
                     "move.w (%[src]),(%[dst])\n\t"
                     "adda.w %[stride],%[src]\n\t"
                     "dbra %[loops],1b\n\t"
                     : [src] "+&a"(cursor), [loops] "+&d"(loops)
                     : [dst] "a"(port), [stride] "d"(stride_bytes)
                     : "cc", "memory");
}

#else
#define UNSIGNED_M68K_VRAM_ASM_ACTIVE 0
#endif

#endif
