/**
 * @file palette.h
 * @brief Logical sprite-palette API routed through the renderer backend.
 *
 * Callers provide complete 16-color palettes; platform-specific upload encoding stays outside
 * the display layer so host tests can exercise presentation policy without direct VRAM access.
 */

#ifndef UNSIGNED_DISPLAY_PALETTE_H
#define UNSIGNED_DISPLAY_PALETTE_H

#include "core/types.h"

#define U_SPRITE_PALETTE_COLOR_COUNT 16

/** Upload one complete sprite palette through the active renderer backend. */
void unsigned_sprite_palette_load(u8 palette, const u16 colors[U_SPRITE_PALETTE_COLOR_COUNT]);

/** Set the backdrop color through the active renderer backend. */
void unsigned_sprite_palette_set_backdrop_color(u16 color);

#endif
