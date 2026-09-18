/**
 * @file background.c
 * @brief Implements camera-driven scrolling background layers.
 */

#include "level/background/background.h"

#include "display/sprite/limits.h"

/** Wrap a scrolling background coordinate into the repeating layer extent. */
static u16 background_wrap(s32 value, u16 width) {
    if (value >= 0 && value < width) {
        return (u16)value;
    }
    if (value < 0 && value >= -(s32)width) {
        return (u16)(value + width);
    }
    if (value >= width && value < (s32)width * 2) {
        return (u16)(value - width);
    }

    value %= width;
    if (value < 0) {
        value += width;
    }
    return (u16)value;
}

/** Apply one fixed-point parallax ratio without runtime division. */
static s32 background_parallax_fixed(s32 delta_x, u8 parallax_fixed) {
    if (parallax_fixed == 0u) {
        return 0;
    }
    if (parallax_fixed == U_BACKGROUND_PARALLAX_ONE) {
        return delta_x * U_BACKGROUND_PARALLAX_ONE;
    }
    return delta_x * parallax_fixed;
}

/** Replace a layer fixed-point scroll accumulator and synchronize its wrapped pixel coordinate. */
static void background_layer_set_scroll(UBackgroundLayer *layer, s32 scroll_fixed) {
    const u16 width = (u16)(layer->definition->width_tiles * 16u);

    layer->scroll_fixed = scroll_fixed;
    layer->scroll_pixels = background_wrap(unsigned_background_fixed_to_pixels(scroll_fixed), width);
}

/** Advance one layer by a camera delta while avoiding a modulo when wrapping is not needed. */
static void background_layer_scroll(UBackgroundLayer *layer, s32 delta_x) {
    const UBackgroundLayerDefinition *definition = layer->definition;
    const u16 width = (u16)(definition->width_tiles * 16u);
    const s32 previous_pixels = unsigned_background_fixed_to_pixels(layer->scroll_fixed);

    layer->scroll_fixed += background_parallax_fixed(delta_x, definition->parallax_fixed);
    const s32 current_pixels = unsigned_background_fixed_to_pixels(layer->scroll_fixed);

    if (current_pixels != previous_pixels) {
        layer->scroll_pixels = background_wrap((s32)layer->scroll_pixels + current_pixels - previous_pixels, width);
    }
}

void unsigned_background_init(UBackground *background) {
    if (background != NULL) {
        *background = (UBackground){ 0 };
    }
}

bool unsigned_background_set_layer(UBackground *background, u8 layer_index, const UBackgroundLayerDefinition *definition) {
    if (background == NULL || layer_index >= UNSIGNED_BACKGROUND_MAX_LAYERS || definition == NULL || definition->width_tiles == 0u ||
        !unsigned_sprite_height_is_valid(definition->height_tiles) || !unsigned_sprite_range_is_valid(definition->first_sprite, 1u)) {
        return false;
    }

    UBackgroundLayer *layer = &background->layers[layer_index];
    *layer = (UBackgroundLayer){
        .definition = definition,
        .active = true,
    };

    /* A layer added after the camera moved must immediately match the current world origin. */
    background_layer_set_scroll(layer, background_parallax_fixed(background->camera_x, definition->parallax_fixed));
    return true;
}

bool unsigned_background_clear_layer(UBackground *background, u8 layer_index) {
    if (background == NULL || layer_index >= UNSIGNED_BACKGROUND_MAX_LAYERS) {
        return false;
    }

    background->layers[layer_index] = (UBackgroundLayer){ 0 };
    return true;
}

void unsigned_background_tick(UBackground *background, s16 camera_x) {
    if (background == NULL) {
        return;
    }

    const s32 delta_x = (s32)camera_x - background->camera_x;
    if (delta_x == 0) {
        return;
    }
    background->camera_x = camera_x;

    for (u8 i = 0u; i < UNSIGNED_BACKGROUND_MAX_LAYERS; ++i) {
        UBackgroundLayer *layer = &background->layers[i];
        if (!layer->active || layer->definition == NULL) {
            continue;
        }
        background_layer_scroll(layer, delta_x);
    }
}
