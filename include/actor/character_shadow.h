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
 * Q8 presentation units (`U_EFFECT_SCALE_ONE` = 100%). The character-shadow adapter keeps
 * the generic scale values for content-facing tuning, then quantizes them once when height changes
 * into the sprite shrink state consumed by the renderer.
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
 * `transform` remains the content-facing Q8 representation of the requested shadow scale.
 * Runtime rendering uses the equivalent cached sprite shrink/offset so stationary shadows do not
 * resample a generic effect every frame.
 */
typedef struct UCharacterShadow {
    USprite sprite;
    const UCharacterShadowDefinition *definition;
    /** Mutable Q8 transform retained as the shadow scale source of truth. */
    UTransformEffect transform;
    /** Q16 reciprocal cached at initialization so height updates need no integer division. */
    u32 height_reciprocal_q16;
} UCharacterShadow;

/**
 * @brief Initialize the ground-shadow sprite and cache the height-to-scale conversion.
 *
 * Height changes later update only quantized shrink/offset state, avoiding generic per-frame effect
 * sampling for a presentation that depends solely on character elevation.
 *
 * @pre `definition->height_for_min_scale > 0`.
 * @pre `min_scale_x` and `min_scale_y` are in [0, U_EFFECT_SCALE_ONE].
 * @pre `definition->sprite` satisfies the USpriteDefinition authoring contract.
 */
void unsigned_character_shadow_init(UCharacterShadow *shadow, const UCharacterShadowDefinition *definition, u16 first_sprite);

/** Update the shadow scale/shrink cache from a non-negative character height in pixels.
 * @pre `shadow` is initialized and `height >= 0`.
 */
void unsigned_character_shadow_set_height(UCharacterShadow *shadow, s16 height);

#endif
