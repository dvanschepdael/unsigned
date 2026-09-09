/**
 * @file effect_shake.c
 * @brief Implements viewport shake effect.
 */

#include "display/viewport/effect_shake.h"

static const Vec2 shake_directions[] = {
    { 1, -1 }, { -1, 1 }, { 1, 1 }, { -1, 0 }, { 0, -1 }, { 1, 0 }, { -1, -1 }, { 0, 1 },
};

/** Computes conservative viewport bounds required by the configured shake effect. */
static UEffectBounds effect_shake_bounds(u8 column_count, const void *context) {
    const UShakeEffect *shake = context;

    (void)column_count;
    if (shake == NULL) {
        return (UEffectBounds){ 0 };
    }

    return (UEffectBounds){
        .offset_x = shake->amplitude_x,
        .offset_y = shake->amplitude_y,
    };
}

void unsigned_effect_set_shake(UEffect *effect, const UShakeEffect *shake, u8 speed) {
    unsigned_effect_set_advanced(effect, unsigned_effect_shake, shake, speed, U_EFFECT_LAYOUT_UNIFORM, effect_shake_bounds);
}

void unsigned_effect_shake(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UShakeEffect *shake = context;
    const Vec2 *direction;

    if (sample == NULL || shake == NULL) {
        return;
    }

    direction = &shake_directions[phase % ARRAY_COUNT(shake_directions)];
    (void)column;
    (void)column_count;
    sample->offset_x = (s16)(shake->amplitude_x * direction->x);
    sample->offset_y = (s16)(shake->amplitude_y * direction->y);
}
