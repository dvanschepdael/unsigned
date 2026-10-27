/**
 * @file palette.c
 * @brief Provides the platform-independent palette API and delegates Neo Geo RAM writes.
 */

#include "display/sprite/palette.h"
#include "system/palette_backend.h"

/** Load all 16 colors of one sprite palette into the active hardware palette bank. */
void unsigned_sprite_palette_load(u8 palette, const u16 colors[U_SPRITE_PALETTE_COLOR_COUNT]) {
    unsigned_system_palette_load(palette, colors);
}

/** Set the color displayed where no sprite/FIX pixel is drawn. */
void unsigned_sprite_palette_set_backdrop_color(u16 color) {
    unsigned_system_palette_set_backdrop(color);
}
