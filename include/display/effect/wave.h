/**
 * @file wave.h
 * @brief Per-column wave effect.
 */
#ifndef UNSIGNED_DISPLAY_EFFECT_WAVE_H
#define UNSIGNED_DISPLAY_EFFECT_WAVE_H

#include "display/effect/effect.h"

/** Applies a phase-shifted displacement to each rendered column. */
typedef struct UWaveEffect {
    s16 amplitude_x;
    s16 amplitude_y;
    s16 zoom_amplitude;
    u8 frequency;
} UWaveEffect;

/** Bind a per-column wave. A NULL configuration clears the destination effect. */
void unsigned_effect_set_wave(UEffect *effect, const UWaveEffect *wave, u8 speed);

#endif
