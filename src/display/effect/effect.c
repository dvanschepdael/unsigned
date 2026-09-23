/**
 * @file effect.c
 * @brief Implements generic sampled display effects and composition.
 */

#include "display/effect/effect.h"

#include "core/math/math.h"

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
    UEffectSample composed = unsigned_effect_identity_sample();
    const u16 first_scale_x = first.scale_x;
    const u16 first_scale_y = first.scale_y;
    const u16 second_scale_x = second.scale_x;
    const u16 second_scale_y = second.scale_y;
    const bool second_scales = second_scale_x != U_EFFECT_SCALE_ONE || second_scale_y != U_EFFECT_SCALE_ONE;

    composed.offset_x = unsigned_math_saturate_s16((s32)first.offset_x + second.offset_x);
    composed.offset_y = unsigned_math_saturate_s16((s32)first.offset_y + second.offset_y);
    composed.zoom_offset = unsigned_math_saturate_s16((s32)first.zoom_offset + second.zoom_offset);
    /* Most presentation effects only translate/flip and keep one or both scale axes at 100%.
     * Avoid a 16x16 multiply on those paths; this matters especially for per-column effects,
     * where composition runs once for every hardware sprite column. */
    if (first_scale_x == U_EFFECT_SCALE_ONE) {
        composed.scale_x = second_scale_x;
    } else if (second_scale_x == U_EFFECT_SCALE_ONE) {
        composed.scale_x = first_scale_x;
    } else {
        composed.scale_x = (u16)(((u32)first_scale_x * second_scale_x + 128u) >> 8u);
    }

    if (first_scale_y == U_EFFECT_SCALE_ONE) {
        composed.scale_y = second_scale_y;
    } else if (second_scale_y == U_EFFECT_SCALE_ONE) {
        composed.scale_y = first_scale_y;
    } else {
        composed.scale_y = (u16)(((u32)first_scale_y * second_scale_y + 128u) >> 8u);
    }
    composed.pivot_x = second_scales ? second.pivot_x : first.pivot_x;
    composed.pivot_y = second_scales ? second.pivot_y : first.pivot_y;
    composed.flags = (u8)(((first.flags ^ second.flags) & (U_EFFECT_SAMPLE_FLIP_X | U_EFFECT_SAMPLE_FLIP_Y)) | ((first.flags | second.flags) & U_EFFECT_SAMPLE_HIDDEN));
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

    return (UEffectBounds){
        .offset_x = x > UINT16_MAX ? UINT16_MAX : (u16)x,
        .offset_y = y > UINT16_MAX ? UINT16_MAX : (u16)y,
    };
}
