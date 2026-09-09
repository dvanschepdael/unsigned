/**
 * @file effect_wave.c
 * @brief Implements viewport wave effect.
 */

#include "display/viewport/effect_wave.h"

/** Evaluates the triangle-wave phase used by the viewport wave effect. */
static s16 effect_wave_triangle(u8 phase) {
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

/** Returns the unsigned magnitude used to bound the wave displacement. */
static u16 effect_wave_abs(s16 value) {
    s32 wide = value;
    if (wide < 0) {
        wide = -wide;
    }
    return wide > UINT16_MAX ? UINT16_MAX : (u16)wide;
}

/** Computes conservative viewport bounds required by the configured wave effect. */
static UEffectBounds effect_wave_bounds(u8 column_count, const void *context) {
    const UWaveEffect *wave = context;

    (void)column_count;
    if (wave == NULL) {
        return (UEffectBounds){ 0 };
    }

    return (UEffectBounds){
        .offset_x = effect_wave_abs(wave->amplitude_x),
        .offset_y = effect_wave_abs(wave->amplitude_y),
    };
}

void unsigned_effect_set_wave(UEffect *effect, const UWaveEffect *wave, u8 speed) {
    unsigned_effect_set_advanced(effect, unsigned_effect_wave, wave, speed, U_EFFECT_LAYOUT_PER_COLUMN, effect_wave_bounds);
}

void unsigned_effect_wave(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context) {
    const UWaveEffect *wave = context;

    if (sample == NULL || wave == NULL) {
        return;
    }

    s16 value = effect_wave_triangle((u8)(phase + (u8)(column * wave->frequency)));
    (void)column_count;
    sample->offset_x = (s16)(((s32)value * wave->amplitude_x) >> 7);
    sample->offset_y = (s16)(((s32)value * wave->amplitude_y) >> 7);
    sample->zoom_offset = (s16)(((s32)value * wave->zoom_amplitude) >> 7);
}
