/**
 * @file effect.c
 * @brief Implements generic sampled display effects and composition.
 */

#include "display/effect/effect.h"

#include "core/math/math.h"
#include "display/effect/effect_compose.h"
#include "display/effect/effect_internal.h"

void unsigned_effect_set_advanced(UEffect *effect, UEffectFunction function, const void *context, u8 speed, UEffectLayout layout, UEffectBoundsFunction bounds) {
    *effect = (UEffect){
        .function = function,
        .bounds = bounds,
        .context = context,
        .speed = speed,
        .layout = layout,
    };
}

void unsigned_effect_set(UEffect *effect, UEffectFunction function, const void *context, u8 speed) {
    unsigned_effect_set_advanced(effect, function, context, speed, U_EFFECT_LAYOUT_PER_COLUMN, NULL);
}

void unsigned_effect_clear(UEffect *effect) {
    *effect = (UEffect){0};
}

void unsigned_effect_tick_frames(UEffect *effect, u16 ticks) {
    if (effect->function != NULL && ticks > 0u) {
        effect->phase = (u8)(effect->phase + (u32)effect->speed * ticks);
    }
}

void unsigned_effect_tick(UEffect *effect) {
    unsigned_effect_tick_frames(effect, 1u);
}

UEffectSample unsigned_effect_identity_sample(void) {
    return (UEffectSample){
        .scale_x = U_EFFECT_SCALE_ONE,
        .scale_y = U_EFFECT_SCALE_ONE,
    };
}

UEffectSample unsigned_effect_sample(const UEffect *effect, u8 column, u8 column_count) {
    UEffectSample sample = unsigned_effect_identity_sample();

    if (effect->function != NULL) {
        effect->function(&sample, column, column_count, effect->phase, effect->context);
    }

    return sample;
}

UEffectSample unsigned_effect_compose_samples(UEffectSample first, UEffectSample second) {
    UEffectSample composed;
    unsigned_effect_compose(&composed, &first, &second);
    return composed;
}

bool unsigned_effect_is_per_column(const UEffect *effect) {
    return effect->function != NULL && effect->layout == U_EFFECT_LAYOUT_PER_COLUMN;
}

bool unsigned_effect_get_bounds(const UEffect *effect, u8 column_count, UEffectBounds *bounds) {
    *bounds = (UEffectBounds){0};
    if (effect->function == NULL) {
        return true;
    }
    if (effect->bounds == NULL) {
        return false;
    }

    *bounds = effect->bounds(column_count, effect->context);
    return true;
}

UEffectBounds unsigned_effect_add_bounds(UEffectBounds first, UEffectBounds second) {
    const u32 x = (u32)first.offset_x + second.offset_x;
    const u32 y = (u32)first.offset_y + second.offset_y;

    return unsigned_effect_saturate_bounds(x, y);
}
