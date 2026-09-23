#include "display/effect/shake.h"

#include "display/effect/effect_internal.h"

static const Vec2 SHAKE_DIRECTIONS[] = {
    {1, -1}, {-1, 1}, {1, 1}, {-1, 0}, {0, -1}, {1, 0}, {-1, -1}, {0, 1},
};

static UEffectBounds shake_bounds(u8 column_count, const void *context) {
    const UShakeEffect *shake = context;
    (void)column_count;

    return (UEffectBounds){.offset_x = shake->amplitude_x, .offset_y = shake->amplitude_y};
}

static void shake_sample(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UShakeEffect *shake = context;
    const Vec2 *direction = &SHAKE_DIRECTIONS[phase % ARRAY_COUNT(SHAKE_DIRECTIONS)];
    (void)column;
    (void)column_count;

    sample->offset_x = (s16)(shake->amplitude_x * direction->x);
    sample->offset_y = (s16)(shake->amplitude_y * direction->y);
}

void unsigned_effect_set_shake(UEffect *effect, const UShakeEffect *shake, u8 speed) {
    unsigned_effect_bind(effect, shake_sample, shake, speed, U_EFFECT_LAYOUT_UNIFORM, shake_bounds);
}
