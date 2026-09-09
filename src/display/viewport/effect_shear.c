/**
 * @file effect_shear.c
 * @brief Implements viewport shear effect.
 */

#include "display/viewport/effect_shear.h"

/** Returns a saturated absolute value used while deriving effect bounds. */
static u16 effect_abs_s16(s16 value) {
    s32 wide = value;
    if (wide < 0) {
        wide = -wide;
    }
    return wide > UINT16_MAX ? UINT16_MAX : (u16)wide;
}

/** Computes conservative viewport bounds required by the configured shear effect. */
static UEffectBounds effect_shear_bounds(u8 column_count, const void *context) {
    const UShearEffect *shear = context;
    const u16 span = column_count > 0u ? (u16)(column_count - 1u) : 0u;

    if (shear == NULL) {
        return (UEffectBounds){ 0 };
    }

    u32 x = ((u32)effect_abs_s16(shear->amplitude_x) * span + 1u) >> 1u;
    u32 y = ((u32)effect_abs_s16(shear->amplitude_y) * span + 1u) >> 1u;

    return (UEffectBounds){
        .offset_x = x > UINT16_MAX ? UINT16_MAX : (u16)x,
        .offset_y = y > UINT16_MAX ? UINT16_MAX : (u16)y,
    };
}

void unsigned_effect_set_shear(UEffect *effect, const UShearEffect *shear, u8 speed) {
    unsigned_effect_set_advanced(effect, unsigned_effect_shear, shear, speed, U_EFFECT_LAYOUT_PER_COLUMN, effect_shear_bounds);
}

void unsigned_effect_shear(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UShearEffect *shear = context;

    if (sample == NULL || shear == NULL) {
        return;
    }

    s16 position = (s16)(((s16)column << 1) - ((s16)column_count - 1));
    (void)phase;
    sample->offset_x = (s16)(((s32)position * shear->amplitude_x) >> 1);
    sample->offset_y = (s16)(((s32)position * shear->amplitude_y) >> 1);
}
