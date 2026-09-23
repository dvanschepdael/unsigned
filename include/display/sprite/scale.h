/**
 * @file scale.h
 * @brief Shared quantized scale helpers used by sprite renderer and Neo Geo backend.
 */
#ifndef UNSIGNED_DISPLAY_SPRITE_SCALE_H
#define UNSIGNED_DISPLAY_SPRITE_SCALE_H

#include "display/effect/effect.h"

#define U_SPRITE_SHRINK_X_FULL 0x0fu
#define U_SPRITE_SHRINK_Y_FULL 0xffu

/** Apply a Q8 multiplier to the horizontal 1..16 hardware width units.
 * @pre `base_shrink <= U_SPRITE_SHRINK_X_FULL` and `scale <= U_EFFECT_SCALE_ONE`.
 */
static inline u8 unsigned_sprite_scale_shrink_x(u8 base_shrink, u16 scale) {
    const u8 base = base_shrink;

    /* Identity scale is the dominant translation-only path and avoids a multiply. */
    if (scale == U_EFFECT_SCALE_ONE) {
        return base;
    }

    const u16 base_units = (u16)base + 1u;
    u16 units = (u16)(((u32)base_units * scale + 128u) >> 8u);
    if (units == 0u) {
        units = 1u;
    }
    return (u8)(units - 1u);
}

/** Apply a Q8 multiplier to the vertical 1..256 hardware height units.
 * @pre `scale <= U_EFFECT_SCALE_ONE`.
 */
static inline u8 unsigned_sprite_scale_shrink_y(u8 base_shrink, u16 scale) {
    if (scale == U_EFFECT_SCALE_ONE) {
        return base_shrink;
    }

    const u16 base_units = (u16)base_shrink + 1u;
    u16 units = (u16)(((u32)base_units * scale + 128u) >> 8u);
    if (units == 0u) {
        units = 1u;
    }
    return (u8)(units - 1u);
}

/** Width in pixels produced by a horizontal shrink value for a chained sprite canvas.
 * @pre `shrink_x <= U_SPRITE_SHRINK_X_FULL`.
 */
static inline u16 unsigned_sprite_scaled_width_pixels(u8 width_tiles, u8 shrink_x) {
    return (u16)((u16)width_tiles * ((u16)shrink_x + 1u));
}

/** Height in pixels produced by a vertical shrink value for the logical sprite canvas. */
static inline u16 unsigned_sprite_scaled_height_pixels(u8 height_tiles, u8 shrink_y) {
    const u16 full_height = (u16)height_tiles * 16u;
    return (u16)(((u32)full_height * ((u16)shrink_y + 1u) + 255u) >> 8u);
}

/** Keep a Q8 pivot stationary while an extent shrinks. @pre `pivot <= U_EFFECT_PIVOT_END`. */
static inline s16 unsigned_sprite_scale_pivot_offset(u16 base_extent, u16 scaled_extent, u16 pivot) {
    if (scaled_extent >= base_extent) {
        return 0;
    }
    return (s16)(((u32)(base_extent - scaled_extent) * pivot + 128u) >> 8u);
}

#endif
