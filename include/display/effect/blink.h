/**
 * @file blink.h
 * @brief Periodic visibility effect.
 */
#ifndef UNSIGNED_DISPLAY_EFFECT_BLINK_H
#define UNSIGNED_DISPLAY_EFFECT_BLINK_H

#include "display/effect/effect.h"

/**
 * Alternates a rendered object between visible and hidden phases.
 * @invariant `visible_phase + hidden_phase` is in [1, 256], matching the 8-bit effect phase period.
 */
typedef struct UBlinkEffect {
    u8 visible_phase;
    u8 hidden_phase;
} UBlinkEffect;

/** Bind periodic visibility. The configuration must remain valid while the effect is bound. @pre `effect` is valid and `blink` satisfies UBlinkEffect invariants. */
void unsigned_effect_set_blink(UEffect *effect, const UBlinkEffect *blink, u8 speed);

#endif
