/**
 * @file scale.h
 * @brief Shared quantized scale helpers used by sprite renderer and Neo Geo backend.
 */
#ifndef UNSIGNED_DISPLAY_SPRITE_SCALE_H
#define UNSIGNED_DISPLAY_SPRITE_SCALE_H

#include "display/effect/effect.h"

#define U_SPRITE_SHRINK_X_FULL 0x0fu
#define U_SPRITE_SHRINK_Y_FULL 0xffu

static inline u16 unsigned_sprite_scale_clamp(u16 scale) {
    return scale > U_EFFECT_SCALE_ONE ? U_EFFECT_SCALE_ONE : scale;
}

/** Apply a Q8 multiplier to the horizontal 1..16 hardware width units. */
static inline u8 unsigned_sprite_scale_shrink_x(u8 base_shrink, u16 scale) {
    const u16 base_units = (u16)(base_shrink & U_SPRITE_SHRINK_X_FULL) + 1u;
    u16 units = (u16)(((u32)base_units * unsigned_sprite_scale_clamp(scale) + 128u) >> 8u);
    if (units == 0u) {
        units = 1u;
    } else if (units > 16u) {
        units = 16u;
    }
    return (u8)(units - 1u);
}

/** Apply a Q8 multiplier to the vertical 1..256 hardware height units. */
static inline u8 unsigned_sprite_scale_shrink_y(u8 base_shrink, u16 scale) {
    const u16 base_units = (u16)base_shrink + 1u;
    u16 units = (u16)(((u32)base_units * unsigned_sprite_scale_clamp(scale) + 128u) >> 8u);
    if (units == 0u) {
        units = 1u;
    } else if (units > 256u) {
        units = 256u;
    }
    return (u8)(units - 1u);
}

/** Width in pixels produced by a horizontal shrink value for a chained sprite canvas. */
static inline u16 unsigned_sprite_scaled_width_pixels(u8 width_tiles, u8 shrink_x) {
    return (u16)((u16)width_tiles * ((u16)(shrink_x & U_SPRITE_SHRINK_X_FULL) + 1u));
}

/** Height in pixels produced by a vertical shrink value for the logical sprite canvas. */
static inline u16 unsigned_sprite_scaled_height_pixels(u8 height_tiles, u8 shrink_y) {
    const u16 full_height = (u16)height_tiles * 16u;
    return (u16)(((u32)full_height * ((u16)shrink_y + 1u) + 255u) >> 8u);
}

/** Keep a Q8 pivot stationary while an extent shrinks. */
static inline s16 unsigned_sprite_scale_pivot_offset(u16 base_extent, u16 scaled_extent, u16 pivot) {
    if (scaled_extent >= base_extent) {
        return 0;
    }
    const u16 clamped_pivot = pivot > U_EFFECT_PIVOT_END ? U_EFFECT_PIVOT_END : pivot;
    return (s16)(((u32)(base_extent - scaled_extent) * clamped_pivot + 128u) >> 8u);
}

#endif
