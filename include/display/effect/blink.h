/**
 * @file blink.h
 * @brief Periodic visibility effect.
 */
#ifndef UNSIGNED_DISPLAY_EFFECT_BLINK_H
#define UNSIGNED_DISPLAY_EFFECT_BLINK_H

#include "display/effect/effect.h"

/** Alternates a rendered object between visible and hidden phases. */
typedef struct UBlinkEffect {
    u8 visible_phase;
    u8 hidden_phase;
} UBlinkEffect;

/** Bind periodic visibility. A NULL configuration clears the destination effect. */
void unsigned_effect_set_blink(UEffect *effect, const UBlinkEffect *blink, u8 speed);

#endif
