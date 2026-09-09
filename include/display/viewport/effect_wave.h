/**
 * @file effect_wave.h
 * @brief Viewport wave effect.
 */

#ifndef UNSIGNED_DISPLAY_VIEWPORT_EFFECT_WAVE_H
#define UNSIGNED_DISPLAY_VIEWPORT_EFFECT_WAVE_H

#include "display/viewport/effect.h"

typedef struct UWaveEffect {
    s16 amplitude_x;
    s16 amplitude_y;
    s16 zoom_amplitude;
    u8 frequency;
} UWaveEffect;

/**
 * @brief Sets wave on the effect.
 *
 * @param effect Viewport effect state to configure, advance or sample.
 * @param wave Wave-effect parameters retained by the generic viewport effect.
 * @param speed Effect phase increment applied each frame.
 */
void unsigned_effect_set_wave(UEffect *effect, const UWaveEffect *wave, u8 speed);

/**
 * @brief Applies the configured wave offset to one viewport effect sample.
 *
 * @param sample Effect sample receiving per-column transform adjustments.
 * @param column Viewport/background hardware column index being sampled or updated.
 * @param column_count Number of columns participating in the effect/render pass.
 * @param phase Current effect/runtime phase supplied to the callback.
 * @param context Opaque caller context passed back to callbacks.
 */
void unsigned_effect_wave(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context);

#endif
