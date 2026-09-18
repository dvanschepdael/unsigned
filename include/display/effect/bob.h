/**
 * @file bob.h
 * @brief Smooth translation oscillation effect.
 */
#ifndef UNSIGNED_DISPLAY_EFFECT_BOB_H
#define UNSIGNED_DISPLAY_EFFECT_BOB_H

#include "display/effect/effect.h"

/** Moves presentation around its logical position without changing gameplay coordinates. */
typedef struct UBobEffect {
    s16 amplitude_x;
    s16 amplitude_y;
} UBobEffect;

/** Bind a smooth translation oscillation. A NULL configuration clears the destination effect. */
void unsigned_effect_set_bob(UEffect *effect, const UBobEffect *bob, u8 speed);

#endif
