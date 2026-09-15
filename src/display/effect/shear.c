#include "display/effect/shear.h"

#include "display/effect/effect_internal.h"

static UEffectBounds shear_bounds(u8 column_count, const void *context) {
    const UShearEffect *shear = context;
    const u16 span = column_count > 0u ? (u16)(column_count - 1u) : 0u;
    const u32 x = ((u32)unsigned_effect_abs_s16(shear->amplitude_x) * span + 1u) >> 1u;
    const u32 y = ((u32)unsigned_effect_abs_s16(shear->amplitude_y) * span + 1u) >> 1u;

    return (UEffectBounds){
        .offset_x = x > UINT16_MAX ? UINT16_MAX : (u16)x,
        .offset_y = y > UINT16_MAX ? UINT16_MAX : (u16)y,
    };
}

static void shear_sample(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UShearEffect *shear = context;
    const s16 position = (s16)(((s16)column << 1) - ((s16)column_count - 1));
    (void)phase;

    sample->offset_x = (s16)(((s32)position * shear->amplitude_x) >> 1);
    sample->offset_y = (s16)(((s32)position * shear->amplitude_y) >> 1);
}

void unsigned_effect_set_shear(UEffect *effect, const UShearEffect *shear, u8 speed) {
    unsigned_effect_bind(effect, shear_sample, shear, speed, U_EFFECT_LAYOUT_PER_COLUMN, shear_bounds);
}
