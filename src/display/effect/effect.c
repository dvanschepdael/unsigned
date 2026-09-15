/**
 * @file effect.c
 * @brief Implements generic sampled display effects and composition.
 */

#include "display/effect/effect.h"

#include <limits.h>

static s16 effect_saturate_s16(s32 value) {
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    return (s16)value;
}

static u16 effect_clamp_scale(u16 scale) {
    return scale > U_EFFECT_SCALE_ONE ? U_EFFECT_SCALE_ONE : scale;
}

void unsigned_effect_set_advanced(UEffect *effect, UEffectFunction function, const void *context, u8 speed, UEffectLayout layout, UEffectBoundsFunction bounds) {
    if (effect == NULL) {
        return;
    }
    if (function == NULL) {
        *effect = (UEffect){0};
        return;
    }

    *effect = (UEffect){
        .function = function,
        .bounds = bounds,
        .context = context,
        .speed = speed,
        .layout = layout == U_EFFECT_LAYOUT_UNIFORM ? U_EFFECT_LAYOUT_UNIFORM : U_EFFECT_LAYOUT_PER_COLUMN,
    };
}

void unsigned_effect_set(UEffect *effect, UEffectFunction function, const void *context, u8 speed) {
    unsigned_effect_set_advanced(effect, function, context, speed, U_EFFECT_LAYOUT_PER_COLUMN, NULL);
}

void unsigned_effect_clear(UEffect *effect) {
    if (effect != NULL) {
        *effect = (UEffect){0};
    }
}

void unsigned_effect_tick(UEffect *effect) {
    if (effect != NULL && effect->function != NULL) {
        effect->phase = (u8)(effect->phase + effect->speed);
    }
}

UEffectSample unsigned_effect_identity_sample(void) {
    return (UEffectSample){
        .scale_x = U_EFFECT_SCALE_ONE,
        .scale_y = U_EFFECT_SCALE_ONE,
    };
}

UEffectSample unsigned_effect_sample(const UEffect *effect, u8 column, u8 column_count) {
    UEffectSample sample = unsigned_effect_identity_sample();

    if (effect != NULL && effect->function != NULL) {
        effect->function(&sample, column, column_count, effect->phase, effect->context);
        sample.scale_x = effect_clamp_scale(sample.scale_x);
        sample.scale_y = effect_clamp_scale(sample.scale_y);
        if (sample.pivot_x > U_EFFECT_PIVOT_END) {
            sample.pivot_x = U_EFFECT_PIVOT_END;
        }
        if (sample.pivot_y > U_EFFECT_PIVOT_END) {
            sample.pivot_y = U_EFFECT_PIVOT_END;
        }
    }

    return sample;
}

UEffectSample unsigned_effect_compose_samples(UEffectSample first, UEffectSample second) {
    UEffectSample composed = unsigned_effect_identity_sample();
    const bool second_scales = second.scale_x != U_EFFECT_SCALE_ONE || second.scale_y != U_EFFECT_SCALE_ONE;

    composed.offset_x = effect_saturate_s16((s32)first.offset_x + second.offset_x);
    composed.offset_y = effect_saturate_s16((s32)first.offset_y + second.offset_y);
    composed.zoom_offset = effect_saturate_s16((s32)first.zoom_offset + second.zoom_offset);
    composed.scale_x = (u16)(((u32)effect_clamp_scale(first.scale_x) * effect_clamp_scale(second.scale_x) + 128u) >> 8u);
    composed.scale_y = (u16)(((u32)effect_clamp_scale(first.scale_y) * effect_clamp_scale(second.scale_y) + 128u) >> 8u);
    composed.pivot_x = second_scales ? second.pivot_x : first.pivot_x;
    composed.pivot_y = second_scales ? second.pivot_y : first.pivot_y;
    composed.flags = (u8)(((first.flags ^ second.flags) & (U_EFFECT_SAMPLE_FLIP_X | U_EFFECT_SAMPLE_FLIP_Y)) | ((first.flags | second.flags) & U_EFFECT_SAMPLE_HIDDEN));
    return composed;
}

UEffectSample unsigned_effect_sample_composed(const UEffect *first, const UEffect *second, u8 column, u8 column_count) {
    return unsigned_effect_compose_samples(unsigned_effect_sample(first, column, column_count), unsigned_effect_sample(second, column, column_count));
}

bool unsigned_effect_is_per_column(const UEffect *effect) {
    return effect != NULL && effect->function != NULL && effect->layout == U_EFFECT_LAYOUT_PER_COLUMN;
}

bool unsigned_effect_get_bounds(const UEffect *effect, u8 column_count, UEffectBounds *bounds) {
    if (bounds == NULL) {
        return false;
    }

    *bounds = (UEffectBounds){0};
    if (effect == NULL || effect->function == NULL) {
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
