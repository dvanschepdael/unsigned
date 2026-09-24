#ifndef UNSIGNED_DISPLAY_EFFECT_EFFECT_INTERNAL_H
#define UNSIGNED_DISPLAY_EFFECT_EFFECT_INTERNAL_H

#include "display/effect/effect.h"

#include <limits.h>

static inline u16 unsigned_effect_abs_s16(s16 value) {
    s32 wide = value;

    if (wide < 0) {
        wide = -wide;
    }
    return (u16)wide;
}

static inline UEffectBounds unsigned_effect_saturate_bounds(u32 x, u32 y) {
    return (UEffectBounds){
        .offset_x = x > UINT16_MAX ? UINT16_MAX : (u16)x,
        .offset_y = y > UINT16_MAX ? UINT16_MAX : (u16)y,
    };
}

static inline UEffectBounds unsigned_effect_zero_bounds(u8 column_count, const void *context) {
    (void)column_count;
    (void)context;
    return (UEffectBounds){0};
}

static inline void unsigned_effect_bind(UEffect *effect, UEffectFunction function, const void *context, u8 speed, UEffectLayout layout, UEffectBoundsFunction bounds) {
    unsigned_effect_set_advanced(effect, function, context, speed, layout, bounds);
}

#endif
