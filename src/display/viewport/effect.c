/**
 * @file effect.c
 * @brief Implements generic per-column viewport effect sampling.
 */

#include "display/viewport/effect.h"

void unsigned_effect_set_advanced(UEffect *effect, UEffectFunction function, const void *context, u8 speed, UEffectLayout layout, UEffectBoundsFunction bounds) {
    if (effect == NULL) {
        return;
    }

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
    if (effect != NULL) {
        *effect = (UEffect){ 0 };
    }
}

void unsigned_effect_tick(UEffect *effect) {
    if (effect != NULL && effect->function != NULL) {
        effect->phase = (u8)(effect->phase + effect->speed);
    }
}

UEffectSample unsigned_effect_sample(const UEffect *effect, u8 column, u8 column_count) {
    UEffectSample sample = { 0 };

    if (effect != NULL && effect->function != NULL) {
        effect->function(&sample, column, column_count, effect->phase, effect->context);
    }

    return sample;
}

bool unsigned_effect_get_bounds(const UEffect *effect, u8 column_count, UEffectBounds *bounds) {
    if (bounds == NULL) {
        return false;
    }

    *bounds = (UEffectBounds){ 0 };
    if (effect == NULL || effect->function == NULL) {
        return true;
    }
    if (effect->bounds == NULL) {
        return false;
    }

    *bounds = effect->bounds(column_count, effect->context);
    return true;
}
