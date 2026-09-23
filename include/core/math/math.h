/**
 * @file math.h
 * @brief Integer min/max/clamp/interpolation helpers.
 */

#ifndef UNSIGNED_CORE_MATH_H
#define UNSIGNED_CORE_MATH_H

#include "core/types.h"

/**
 * @brief Returns the smaller of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The smaller of `left` and `right`.
 */
s16 unsigned_math_min_s16(s16 left, s16 right);

/**
 * @brief Returns the larger of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The larger of `left` and `right`.
 */
s16 unsigned_math_max_s16(s16 left, s16 right);

/**
 * @brief Clamps an integer value to the inclusive minimum/maximum range.
 *
 * @param value Integer value to clamp.
 * @param min_value Inclusive lower bound.
 * @param max_value Inclusive upper bound.
 * @return `value` clamped to the inclusive [`min_value`, `max_value`] range.
 */
s16 unsigned_math_clamp_s16(s16 value, s16 min_value, s16 max_value);

/**
 * @brief Interpolates between two integer values using the supplied position and duration.
 *
 * @param from Start value of the interpolation.
 * @param to End value of the interpolation.
 * @param position Position used by the calculation or transformed result.
 * @param duration Total duration used by the interpolation/runtime item.
 * @return The interpolated value at `position`; returns `to` once `position >= duration` or when `duration` is zero.
 */
/** @pre `duration > 0`. */
s16 unsigned_math_lerp_s16(s16 from, s16 to, u16 position, u16 duration);

/**
 * @brief Interpolates between two integer values using the supplied position and duration.
 *
 * @param from Start value of the interpolation.
 * @param to End value of the interpolation.
 * @param position Position used by the calculation or transformed result.
 * @param duration Total duration used by the interpolation/runtime item.
 * @return The interpolated value at `position`; returns `to` once `position >= duration` or when `duration` is zero.
 */
/** @pre `duration > 0`. */
u16 unsigned_math_lerp_u16(u16 from, u16 to, u16 position, u16 duration);

/**
 * @brief Saturates the wider signed value to the target 16-bit range.
 *
 * @param value Signed 32-bit value to saturate into the s16 range.
 * @return `value` clamped to the signed 16-bit range.
 */
s16 unsigned_math_saturate_s16(s32 value);

/**
 * @brief Divides a signed 32-bit value by 2^shift with C99 truncation toward zero.
 *
 * This avoids target integer-division instructions on fixed-point hot paths while preserving
 * the exact semantics of signed `/` for negative values. `shift` must be in [0, 30].
 */
static inline s32 unsigned_math_div_pow2_s32(s32 value, u8 shift) {
    if (shift == 0u) {
        return value;
    }

    const u32 magnitude = value < 0 ? (u32)(-(value + 1)) + 1u : (u32)value;
    const s32 quotient = (s32)(magnitude >> shift);
    return value < 0 ? -quotient : quotient;
}

#endif
