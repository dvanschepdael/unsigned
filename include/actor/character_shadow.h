/**
 * @file character_shadow.h
 * @brief Height-responsive ground shadow owned by a character.
 */

#ifndef UNSIGNED_ACTOR_CHARACTER_SHADOW_H
#define UNSIGNED_ACTOR_CHARACTER_SHADOW_H

#include "display/effect/transform.h"
#include "display/sprite/sprite.h"

/**
 * Immutable presentation tuning for one character ground shadow.
 *
 * `anchor` is the shadow center relative to the actor ground position. Scale values use
 * Q8 presentation units (`U_EFFECT_SCALE_ONE` = 100%) and are independent from Neo Geo
 * SCB2 encoding. The character adapter only maps elevation to the generic sprite effect;
 * renderer/backend layers decide how that effect becomes hardware shrink values.
 */
typedef struct UCharacterShadowDefinition {
    const USpriteDefinition *sprite;
    Vec2 anchor;
    u16 height_for_min_scale;
    u16 min_scale_x;
    u16 min_scale_y;
} UCharacterShadowDefinition;

/**
 * Runtime state derived from an immutable shadow definition.
 *
 * The sprite effect is bound to this instance's `transform`; initialize it at its final address
 * and do not copy an initialized shadow by value.
 */
typedef struct UCharacterShadow {
    USprite sprite;
    const UCharacterShadowDefinition *definition;
    /** Mutable generic transform sampled through sprite.effect. */
    UTransformEffect transform;
    /** Q16 reciprocal cached at initialization so height updates need no integer division. */
    u32 height_reciprocal_q16;
} UCharacterShadow;

/** Initialize a character shadow, bind its generic sprite transform and start animation zero. */
bool unsigned_character_shadow_init(UCharacterShadow *shadow, const UCharacterShadowDefinition *definition, u16 first_sprite);

/** Update the generic shadow scale from a non-negative character height in pixels. */
void unsigned_character_shadow_set_height(UCharacterShadow *shadow, s16 height);

#endif
