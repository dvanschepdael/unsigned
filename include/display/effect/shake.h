/**
 * @file shake.h
 * @brief Uniform screen-space shake effect.
 */
#ifndef UNSIGNED_DISPLAY_EFFECT_SHAKE_H
#define UNSIGNED_DISPLAY_EFFECT_SHAKE_H

#include "display/effect/effect.h"

/** Applies the same pseudo-random-looking displacement to every rendered column. */
typedef struct UShakeEffect {
    u8 amplitude_x;
    u8 amplitude_y;
} UShakeEffect;

/** Bind a uniform shake. The configuration must remain valid while the effect is bound. @pre `effect` and the effect configuration are valid. */
void unsigned_effect_set_shake(UEffect *effect, const UShakeEffect *shake, u8 speed);

#endif
