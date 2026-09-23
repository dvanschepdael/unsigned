/**
 * @file background.h
 * @brief Scrolling background layer definitions and runtime state.
 */

#ifndef UNSIGNED_LEVEL_BACKGROUND_H
#define UNSIGNED_LEVEL_BACKGROUND_H

#include "core/types.h"
#include "display/sprite/config.h"
#include "level/background/config.h"

#define U_BACKGROUND_PARALLAX_FRACTION_BITS 3
#define U_BACKGROUND_PARALLAX_ONE (1 << U_BACKGROUND_PARALLAX_FRACTION_BITS)

/** Convert signed fixed-point parallax scroll to whole pixels with truncation toward zero. */
static inline s32 unsigned_background_fixed_to_pixels(s32 value) {
    if (value >= 0) {
        return (s32)((u32)value >> U_BACKGROUND_PARALLAX_FRACTION_BITS);
    }

    return -(s32)(((u32)(-(value + 1)) + 1u) >> U_BACKGROUND_PARALLAX_FRACTION_BITS);
}

/**
 * Immutable presentation definition for one repeating parallax layer.
 *
 * @invariant `width_tiles > 0` and `1 <= height_tiles <= 32`.
 * @invariant `first_sprite` names a usable Neo Geo sprite column.
 * @invariant `auto_animation` contains only bits from `U_SPRITE_AUTO_ANIMATION_MASK`.
 */
typedef struct UBackgroundLayerDefinition {
    u16 first_sprite;
    u16 first_tile;
    u8 palette;
    u8 width_tiles;
    u8 height_tiles;
    u8 parallax_fixed;
    Vec2 screen_position;
    u8 auto_animation;
} UBackgroundLayerDefinition;

typedef struct UBackgroundLayer {
    /** NULL means the layer is inactive; active layers always own an immutable definition. */
    const UBackgroundLayerDefinition *definition;
    s32 scroll_fixed;
    u16 scroll_pixels;
} UBackgroundLayer;

typedef struct UBackground {
    UBackgroundLayer layers[UNSIGNED_BACKGROUND_MAX_LAYERS];
    /** Last camera X consumed by parallax; backgrounds never own or follow gameplay actors. */
    s16 camera_x;
} UBackground;

/** Clear all background layers and reset camera-derived scrolling.
 * @pre `background` points to writable runtime storage.
 */
void unsigned_background_init(UBackground *background);

/**
 * @brief Installs or replaces one camera-scrolling presentation layer.
 * @pre `background` and `definition` are valid.
 * @pre `layer_index < UNSIGNED_BACKGROUND_MAX_LAYERS`.
 * @pre `definition` satisfies the `UBackgroundLayerDefinition` content invariants.
 */
void unsigned_background_set_layer(UBackground *background, u8 layer_index, const UBackgroundLayerDefinition *definition);

/**
 * @brief Deactivates one presentation layer without affecting the remaining parallax state.
 * @pre `layer_index < UNSIGNED_BACKGROUND_MAX_LAYERS`.
 */
void unsigned_background_clear_layer(UBackground *background, u8 layer_index);

/**
 * Advance parallax from the current camera X coordinate.
 * Passing the absolute camera coordinate keeps level orchestration simple and prevents gameplay
 * actors from becoming presentation dependencies. No layer work is performed when X is unchanged.
 */
void unsigned_background_tick(UBackground *background, s16 camera_x);

#endif
