/**
 * @file math.c
 * @brief Implements integer min/max/clamp/interpolation helpers.
 */

#include "core/math/math.h"

#include <limits.h>

u8 unsigned_math_min_u8(u8 left, u8 right) {
    return left < right ? left : right;
}

u16 unsigned_math_min_u16(u16 left, u16 right) {
    return left < right ? left : right;
}

u32 unsigned_math_min_u32(u32 left, u32 right) {
    return left < right ? left : right;
}

s8 unsigned_math_min_s8(s8 left, s8 right) {
    return left < right ? left : right;
}

s16 unsigned_math_min_s16(s16 left, s16 right) {
    return left < right ? left : right;
}

s32 unsigned_math_min_s32(s32 left, s32 right) {
    return left < right ? left : right;
}

u8 unsigned_math_max_u8(u8 left, u8 right) {
    return left > right ? left : right;
}

u16 unsigned_math_max_u16(u16 left, u16 right) {
    return left > right ? left : right;
}

u32 unsigned_math_max_u32(u32 left, u32 right) {
    return left > right ? left : right;
}

s8 unsigned_math_max_s8(s8 left, s8 right) {
    return left > right ? left : right;
}

s16 unsigned_math_max_s16(s16 left, s16 right) {
    return left > right ? left : right;
}

s32 unsigned_math_max_s32(s32 left, s32 right) {
    return left > right ? left : right;
}

u8 unsigned_math_clamp_u8(u8 value, u8 min_value, u8 max_value) {
    return unsigned_math_min_u8(unsigned_math_max_u8(value, min_value), max_value);
}

u16 unsigned_math_clamp_u16(u16 value, u16 min_value, u16 max_value) {
    return unsigned_math_min_u16(unsigned_math_max_u16(value, min_value), max_value);
}

u32 unsigned_math_clamp_u32(u32 value, u32 min_value, u32 max_value) {
    return unsigned_math_min_u32(unsigned_math_max_u32(value, min_value), max_value);
}

s8 unsigned_math_clamp_s8(s8 value, s8 min_value, s8 max_value) {
    return unsigned_math_min_s8(unsigned_math_max_s8(value, min_value), max_value);
}

s16 unsigned_math_clamp_s16(s16 value, s16 min_value, s16 max_value) {
    return unsigned_math_min_s16(unsigned_math_max_s16(value, min_value), max_value);
}

s32 unsigned_math_clamp_s32(s32 value, s32 min_value, s32 max_value) {
    return unsigned_math_min_s32(unsigned_math_max_s32(value, min_value), max_value);
}

u8 unsigned_math_abs_s8(s8 value) {
    return value < 0 ? (u8)(-(s16)value) : (u8)value;
}

u16 unsigned_math_abs_s16(s16 value) {
    return value < 0 ? (u16)(-(s32)value) : (u16)value;
}

u32 unsigned_math_abs_s32(s32 value) {
    return value < 0 ? (u32)(-(value + 1)) + 1u : (u32)value;
}

s8 unsigned_math_sign_s8(s8 value) {
    return (s8)((value > 0) - (value < 0));
}

s8 unsigned_math_sign_s16(s16 value) {
    return (s8)((value > 0) - (value < 0));
}

s8 unsigned_math_sign_s32(s32 value) {
    return (s8)((value > 0) - (value < 0));
}

/** Interpolates between two integer values using the supplied position and duration. */
static u32 unsigned_math_lerp_step(s32 delta, u16 position, u16 duration) {
    const u32 magnitude = unsigned_math_abs_s32(delta);
    return (magnitude * position) / duration;
}

s16 unsigned_math_lerp_s16(s16 from, s16 to, u16 position, u16 duration) {
    if (duration == 0u || position >= duration) {
        return to;
    }
    if (position == 0u) {
        return from;
    }

    s32 delta = (s32)to - from;
    u32 step = unsigned_math_lerp_step(delta, position, duration);
    return delta < 0 ? (s16)((s32)from - (s32)step) : (s16)((s32)from + (s32)step);
}

u16 unsigned_math_lerp_u16(u16 from, u16 to, u16 position, u16 duration) {
    if (duration == 0u || position >= duration) {
        return to;
    }
    if (position == 0u) {
        return from;
    }

    s32 delta = (s32)to - from;
    u32 step = unsigned_math_lerp_step(delta, position, duration);
    return delta < 0 ? (u16)((s32)from - (s32)step) : (u16)((s32)from + (s32)step);
}

s16 unsigned_math_saturate_s16(s32 value) {
    return (s16)unsigned_math_clamp_s32(value, INT16_MIN, INT16_MAX);
}
