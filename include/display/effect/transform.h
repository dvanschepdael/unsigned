/**
 * @file transform.h
 * @brief Mutable presentation transform effect.
 */
#ifndef UNSIGNED_DISPLAY_EFFECT_TRANSFORM_H
#define UNSIGNED_DISPLAY_EFFECT_TRANSFORM_H

#include "display/effect/effect.h"

/**
 * Mutable presentation transform for a sprite.
 *
 * Use this when gameplay owns the value directly, for example a shadow whose scale depends on
 * character height. Scale and pivot values use Q8 units: 256 is 100%, 128 is 50%.
 */
typedef struct UTransformEffect {
    s16 offset_x;
    s16 offset_y;
    u16 scale_x;
    u16 scale_y;
    u16 pivot_x;
    u16 pivot_y;
    bool flip_x;
    bool flip_y;
    bool hidden;
} UTransformEffect;

/** Initialize a mutable transform to identity: visible, unflipped, untranslated and 100% scale. */
void unsigned_effect_transform_init(UTransformEffect *transform);

/** Update the transform translation in pixels. */
void unsigned_effect_transform_set_translation(UTransformEffect *transform, s16 offset_x, s16 offset_y);

/** Update Q8 scale and pivot; values above 256 are clamped to the Neo Geo enlargement limit. */
void unsigned_effect_transform_set_scale(UTransformEffect *transform, u16 scale_x, u16 scale_y, u16 pivot_x, u16 pivot_y);

/** Update temporary presentation mirroring without changing gameplay facing. */
void unsigned_effect_transform_set_flip(UTransformEffect *transform, bool flip_x, bool flip_y);

/** Hide/show presentation without releasing sprite allocation or stopping animation. */
void unsigned_effect_transform_set_visible(UTransformEffect *transform, bool visible);

/** Bind a mutable transform. A NULL configuration clears the destination effect. */
void unsigned_effect_set_transform(UEffect *effect, const UTransformEffect *transform);

#endif
