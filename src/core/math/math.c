/**
 * @file math.c
 * @brief Implements integer min/max/clamp/interpolation helpers.
 */

#include "core/math/math.h"

#include <limits.h>

s16 unsigned_math_min_s16(s16 left, s16 right) {
    return left < right ? left : right;
}

s16 unsigned_math_max_s16(s16 left, s16 right) {
    return left > right ? left : right;
}

s16 unsigned_math_clamp_s16(s16 value, s16 min_value, s16 max_value) {
    return unsigned_math_min_s16(unsigned_math_max_s16(value, min_value), max_value);
}

/** Interpolates between two integer values using the supplied position and duration. */
static u32 unsigned_math_lerp_step(s32 delta, u16 position, u16 duration) {
    const u32 magnitude = delta < 0 ? (u32)(-(delta + 1)) + 1u : (u32)delta;
    return (magnitude * position) / duration;
}

s16 unsigned_math_lerp_s16(s16 from, s16 to, u16 position, u16 duration) {
    if (position >= duration) {
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
    if (position >= duration) {
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
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    return (s16)value;
}
