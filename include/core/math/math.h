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
u8 unsigned_math_min_u8(u8 left, u8 right);

/**
 * @brief Returns the smaller of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The smaller of `left` and `right`.
 */
u16 unsigned_math_min_u16(u16 left, u16 right);

/**
 * @brief Returns the smaller of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The smaller of `left` and `right`.
 */
u32 unsigned_math_min_u32(u32 left, u32 right);

/**
 * @brief Returns the smaller of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The smaller of `left` and `right`.
 */
s8 unsigned_math_min_s8(s8 left, s8 right);

/**
 * @brief Returns the smaller of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The smaller of `left` and `right`.
 */
s16 unsigned_math_min_s16(s16 left, s16 right);

/**
 * @brief Returns the smaller of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The smaller of `left` and `right`.
 */
s32 unsigned_math_min_s32(s32 left, s32 right);

/**
 * @brief Returns the larger of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The larger of `left` and `right`.
 */
u8 unsigned_math_max_u8(u8 left, u8 right);

/**
 * @brief Returns the larger of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The larger of `left` and `right`.
 */
u16 unsigned_math_max_u16(u16 left, u16 right);

/**
 * @brief Returns the larger of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The larger of `left` and `right`.
 */
u32 unsigned_math_max_u32(u32 left, u32 right);

/**
 * @brief Returns the larger of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The larger of `left` and `right`.
 */
s8 unsigned_math_max_s8(s8 left, s8 right);

/**
 * @brief Returns the larger of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The larger of `left` and `right`.
 */
s16 unsigned_math_max_s16(s16 left, s16 right);

/**
 * @brief Returns the larger of the two integer operands.
 *
 * @param left Left operand.
 * @param right Right operand.
 * @return The larger of `left` and `right`.
 */
s32 unsigned_math_max_s32(s32 left, s32 right);

/**
 * @brief Clamps an integer value to the inclusive minimum/maximum range.
 *
 * @param value Integer value to clamp.
 * @param min_value Inclusive lower bound.
 * @param max_value Inclusive upper bound.
 * @return `value` clamped to the inclusive [`min_value`, `max_value`] range.
 */
u8 unsigned_math_clamp_u8(u8 value, u8 min_value, u8 max_value);

/**
 * @brief Clamps an integer value to the inclusive minimum/maximum range.
 *
 * @param value Integer value to clamp.
 * @param min_value Inclusive lower bound.
 * @param max_value Inclusive upper bound.
 * @return `value` clamped to the inclusive [`min_value`, `max_value`] range.
 */
u16 unsigned_math_clamp_u16(u16 value, u16 min_value, u16 max_value);

/**
 * @brief Clamps an integer value to the inclusive minimum/maximum range.
 *
 * @param value Integer value to clamp.
 * @param min_value Inclusive lower bound.
 * @param max_value Inclusive upper bound.
 * @return `value` clamped to the inclusive [`min_value`, `max_value`] range.
 */
u32 unsigned_math_clamp_u32(u32 value, u32 min_value, u32 max_value);

/**
 * @brief Clamps an integer value to the inclusive minimum/maximum range.
 *
 * @param value Integer value to clamp.
 * @param min_value Inclusive lower bound.
 * @param max_value Inclusive upper bound.
 * @return `value` clamped to the inclusive [`min_value`, `max_value`] range.
 */
s8 unsigned_math_clamp_s8(s8 value, s8 min_value, s8 max_value);

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
 * @brief Clamps an integer value to the inclusive minimum/maximum range.
 *
 * @param value Integer value to clamp.
 * @param min_value Inclusive lower bound.
 * @param max_value Inclusive upper bound.
 * @return `value` clamped to the inclusive [`min_value`, `max_value`] range.
 */
s32 unsigned_math_clamp_s32(s32 value, s32 min_value, s32 max_value);

/**
 * @brief Returns the unsigned magnitude of the signed integer value.
 *
 * @param value Signed integer whose magnitude is requested.
 * @return The magnitude of `value` represented as `u8`.
 */
u8 unsigned_math_abs_s8(s8 value);

/**
 * @brief Returns the unsigned magnitude of the signed integer value.
 *
 * @param value Signed integer whose magnitude is requested.
 * @return The magnitude of `value` represented as `u16`.
 */
u16 unsigned_math_abs_s16(s16 value);

/**
 * @brief Returns the unsigned magnitude of the signed integer value.
 *
 * @param value Signed integer whose magnitude is requested.
 * @return The magnitude of `value` represented as `u32`.
 */
u32 unsigned_math_abs_s32(s32 value);

/**
 * @brief Returns -1, 0 or 1 according to the sign of the integer value.
 *
 * @param value Signed integer whose sign is classified.
 * @return -1 for a negative value, 0 for zero, or 1 for a positive value.
 */
s8 unsigned_math_sign_s8(s8 value);

/**
 * @brief Returns -1, 0 or 1 according to the sign of the integer value.
 *
 * @param value Signed integer whose sign is classified.
 * @return -1 for a negative value, 0 for zero, or 1 for a positive value.
 */
s8 unsigned_math_sign_s16(s16 value);

/**
 * @brief Returns -1, 0 or 1 according to the sign of the integer value.
 *
 * @param value Signed integer whose sign is classified.
 * @return -1 for a negative value, 0 for zero, or 1 for a positive value.
 */
s8 unsigned_math_sign_s32(s32 value);

/**
 * @brief Interpolates between two integer values using the supplied position and duration.
 *
 * @param from Start value of the interpolation.
 * @param to End value of the interpolation.
 * @param position Position used by the calculation or transformed result.
 * @param duration Total duration used by the interpolation/runtime item.
 * @return The interpolated value at `position`; returns `to` once `position >= duration` or when `duration` is zero.
 */
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
u16 unsigned_math_lerp_u16(u16 from, u16 to, u16 position, u16 duration);

/**
 * @brief Saturates the wider signed value to the target 16-bit range.
 *
 * @param value Signed 32-bit value to saturate into the s16 range.
 * @return `value` clamped to the signed 16-bit range.
 */
s16 unsigned_math_saturate_s16(s32 value);

#endif
