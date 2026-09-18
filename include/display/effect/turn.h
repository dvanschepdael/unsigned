/**
 * @file turn.h
 * @brief Looping pseudo-3D turn effect.
 */
#ifndef UNSIGNED_DISPLAY_EFFECT_TURN_H
#define UNSIGNED_DISPLAY_EFFECT_TURN_H

#include "display/effect/effect.h"

/** Axis used by the pseudo-3D turn effect. */
typedef enum UTurnAxis {
    /** Turn around a vertical axis: horizontal squash followed by horizontal mirroring. */
    U_TURN_AXIS_Y = 0,
    /** Turn around a horizontal axis: vertical squash followed by vertical mirroring. */
    U_TURN_AXIS_X = 1,
} UTurnAxis;

/** Creates a looping card-turn illusion by shrinking one axis and mirroring the back half. */
typedef struct UTurnEffect {
    u16 min_scale;
    u16 pivot_x;
    u16 pivot_y;
    UTurnAxis axis;
} UTurnEffect;

/** Bind a looping pseudo-3D turn. This is squash/mirror, not arbitrary rotation. */
void unsigned_effect_set_turn(UEffect *effect, const UTurnEffect *turn, u8 speed);

#endif
