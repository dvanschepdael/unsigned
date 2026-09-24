/**
 * @file save_internal.h
 * @brief Internal shared primitives for the save subsystem and its platform backend.
 */

#ifndef UNSIGNED_SAVE_INTERNAL_H
#define UNSIGNED_SAVE_INTERNAL_H

#include "core/types.h"

/** Copy a bounded save payload without introducing a libc dependency. */
static inline void unsigned_save_copy_bytes(u8 *dst, const u8 *src, u32 size) {
    while (size-- != 0u) {
        *dst++ = *src++;
    }
}

/** Encode one 16-bit save field in stable big-endian byte order. */
static inline void unsigned_save_write_u16_be(u8 *dst, u16 value) {
    dst[0] = (u8)(value >> 8u);
    dst[1] = (u8)value;
}

/** Decode one 16-bit save field from stable big-endian byte order. */
static inline u16 unsigned_save_read_u16_be(const u8 *src) {
    return (u16)(((u16)src[0] << 8u) | (u16)src[1]);
}

#endif
