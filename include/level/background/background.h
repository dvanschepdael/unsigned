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
    const UBackgroundLayerDefinition *definition;
    s32 scroll_fixed;
    u16 scroll_pixels;
    bool active;
} UBackgroundLayer;

typedef struct UBackground {
    UBackgroundLayer layers[UNSIGNED_BACKGROUND_MAX_LAYERS];
    /** Last camera X consumed by parallax; backgrounds never own or follow gameplay actors. */
    s16 camera_x;
} UBackground;

/** Clear all background layers and reset camera-derived scrolling. */
void unsigned_background_init(UBackground *background);

/** Install or replace one active scrolling layer definition. */
bool unsigned_background_set_layer(UBackground *background, u8 layer_index, const UBackgroundLayerDefinition *definition);

/** Deactivate and clear one background layer slot. */
bool unsigned_background_clear_layer(UBackground *background, u8 layer_index);

/**
 * Advance parallax from the current camera X coordinate.
 * Passing the absolute camera coordinate keeps level orchestration simple and prevents gameplay
 * actors from becoming presentation dependencies. No layer work is performed when X is unchanged.
 */
void unsigned_background_tick(UBackground *background, s16 camera_x);

#endif
