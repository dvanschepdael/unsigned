#include "display/effect/bob.h"

#include "display/effect/effect_internal.h"

static s16 bob_triangle(u8 phase) {
    if (phase < 64u) {
        return (s16)phase * 2;
    }
    if (phase < 192u) {
        return (s16)(128 - (s16)(phase - 64u) * 2);
    }
    return (s16)(-128 + (s16)(phase - 192u) * 2);
}

static UEffectBounds bob_bounds(u8 column_count, const void *context) {
    const UBobEffect *bob = context;
    (void)column_count;

    return (UEffectBounds){
        .offset_x = unsigned_effect_abs_s16(bob->amplitude_x),
        .offset_y = unsigned_effect_abs_s16(bob->amplitude_y),
    };
}

static void bob_sample(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UBobEffect *bob = context;
    const s16 value = bob_triangle(phase);
    (void)column;
    (void)column_count;

    sample->offset_x = (s16)(((s32)value * bob->amplitude_x) >> 7u);
    sample->offset_y = (s16)(((s32)value * bob->amplitude_y) >> 7u);
}

void unsigned_effect_set_bob(UEffect *effect, const UBobEffect *bob, u8 speed) {
    unsigned_effect_bind(effect, bob_sample, bob, speed, U_EFFECT_LAYOUT_UNIFORM, bob_bounds);
}
