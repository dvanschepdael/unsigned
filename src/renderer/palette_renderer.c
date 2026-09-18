/**
 * @file palette_renderer.c
 * @brief Provides the platform-independent palette API and delegates Neo Geo RAM writes.
 */

#include "display/sprite/palette.h"
#include "system/renderer_backend.h"

/** Load all 16 colors of one sprite palette into the active hardware palette bank. */
void unsigned_sprite_palette_load(u8 palette, const u16 colors[U_SPRITE_PALETTE_COLOR_COUNT]) {
    if (colors == NULL) {
        return;
    }
    unsigned_palette_backend_load(palette, colors);
}

/** Set the color displayed where no sprite/FIX pixel is drawn. */
void unsigned_sprite_palette_set_backdrop_color(u16 color) {
    unsigned_palette_backend_set_backdrop_color(color);
}
