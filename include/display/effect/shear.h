/**
 * @file shear.h
 * @brief Per-column shear effect.
 */
#ifndef UNSIGNED_DISPLAY_EFFECT_SHEAR_H
#define UNSIGNED_DISPLAY_EFFECT_SHEAR_H

#include "display/effect/effect.h"

/** Offsets each hardware column progressively to create a tilt/shear illusion. */
typedef struct UShearEffect {
    s16 amplitude_x;
    s16 amplitude_y;
} UShearEffect;

/** Bind a per-column shear. The configuration must remain valid while the effect is bound. @pre `effect` and the effect configuration are valid. */
void unsigned_effect_set_shear(UEffect *effect, const UShearEffect *shear, u8 speed);

#endif
