/**
 * @file effect_shake.h
 * @brief Viewport shake effect.
 */

#ifndef UNSIGNED_DISPLAY_VIEWPORT_EFFECT_SHAKE_H
#define UNSIGNED_DISPLAY_VIEWPORT_EFFECT_SHAKE_H

#include "display/viewport/effect.h"

typedef struct UShakeEffect {
    u8 amplitude_x;
    u8 amplitude_y;
} UShakeEffect;

/**
 * @brief Sets shake on the effect.
 *
 * @param effect Viewport effect state to configure, advance or sample.
 * @param shake Shake-effect parameters retained by the generic viewport effect.
 * @param speed Effect phase increment applied each frame.
 */
void unsigned_effect_set_shake(UEffect *effect, const UShakeEffect *shake, u8 speed);

/**
 * @brief Applies the configured shake offset to one viewport effect sample.
 *
 * @param sample Effect sample receiving per-column transform adjustments.
 * @param column Viewport/background hardware column index being sampled or updated.
 * @param column_count Number of columns participating in the effect/render pass.
 * @param phase Current effect/runtime phase supplied to the callback.
 * @param context Opaque caller context passed back to callbacks.
 */
void unsigned_effect_shake(UEffectSample *sample, u8 column, u8 column_count, u8 phase, const void *context);

#endif
