/**
 * @file background.h
 * @brief Scrolling background layer definitions and runtime state.
 */

#ifndef UNSIGNED_LEVEL_BACKGROUND_H
#define UNSIGNED_LEVEL_BACKGROUND_H

#include "core/types.h"
#include "display/sprite/attributes.h"
#include "level/background/config.h"

#define U_BACKGROUND_PARALLAX_FRACTION_BITS 3
#define U_BACKGROUND_PARALLAX_ONE (1 << U_BACKGROUND_PARALLAX_FRACTION_BITS)

/**
 * @brief Converts the signed parallax fixed-point accumulator to whole pixels with truncation toward zero.
 *
 * @param value Signed fixed-point scroll value using U_BACKGROUND_PARALLAX_FRACTION_BITS fractional bits.
 * @return Whole-pixel scroll coordinate.
 */
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
    const Vec2 *follow_target;
    s16 follow_origin_x;
    s16 follow_last_x;
} UBackground;

/**
 * @brief Clears all background layers and removes any camera/follow target.
 *
 * @param background Background runtime to initialize; NULL is ignored.
 */
void unsigned_background_init(UBackground *background);
/**
 * @brief Installs or replaces one active scrolling layer definition.
 *
 * @param background Background runtime that owns the layer state.
 * @param layer_index Layer slot in [0, UNSIGNED_BACKGROUND_MAX_LAYERS).
 * @param definition Persistent caller-owned layer definition; must remain valid while the layer is active.
 * @return true when index/dimensions/sprite range are valid; false otherwise.
 */
bool unsigned_background_set_layer(UBackground *background, u8 layer_index, const UBackgroundLayerDefinition *definition);

/**
 * @brief Deactivates and clears one background layer slot.
 *
 * @param background Background runtime that owns the layer state.
 * @param layer_index Layer slot to clear.
 * @return true when background and layer_index are valid; false otherwise.
 */
bool unsigned_background_clear_layer(UBackground *background, u8 layer_index);

/**
 * @brief Binds background parallax to a caller-owned world position and resets accumulated scroll.
 *
 * @param background Background runtime to bind.
 * @param target Persistent position pointer sampled by unsigned_background_tick(); caller retains ownership.
 * @return true when both pointers are valid; false otherwise.
 */
bool unsigned_background_set_follow_target(UBackground *background, const Vec2 *target);

/**
 * @brief Advances active layer scroll from horizontal movement of the bound follow target.
 *
 * @param background Background runtime to advance once per engine frame.
 */
void unsigned_background_tick(UBackground *background);

#endif
