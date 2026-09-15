#include "display/effect/wave.h"

#include "display/effect/effect_internal.h"

static s16 wave_triangle(u8 phase) {
    const u8 value = phase & 63u;

    if (value < 16u) {
        return (s16)value * 8;
    }
    if (value < 32u) {
        return (s16)(31u - value) * 8;
    }
    if (value < 48u) {
        return -((s16)(value - 32u) * 8);
    }
    return -((s16)(63u - value) * 8);
}

static UEffectBounds wave_bounds(u8 column_count, const void *context) {
    const UWaveEffect *wave = context;
    (void)column_count;

    return (UEffectBounds){
        .offset_x = unsigned_effect_abs_s16(wave->amplitude_x),
        .offset_y = unsigned_effect_abs_s16(wave->amplitude_y),
    };
}

static void wave_sample(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UWaveEffect *wave = context;
    const s16 value = wave_triangle((u8)(phase + (u8)(column * wave->frequency)));
    (void)column_count;

    sample->offset_x = (s16)(((s32)value * wave->amplitude_x) >> 7);
    sample->offset_y = (s16)(((s32)value * wave->amplitude_y) >> 7);
    sample->zoom_offset = (s16)(((s32)value * wave->zoom_amplitude) >> 7);
}

void unsigned_effect_set_wave(UEffect *effect, const UWaveEffect *wave, u8 speed) {
    unsigned_effect_bind(effect, wave_sample, wave, speed, U_EFFECT_LAYOUT_PER_COLUMN, wave_bounds);
}
