/**
 * @file palette_backend.c
 * @brief Implements Neo Geo palette RAM backend.
 */

#include "display/sprite/palette.h"
#include "system/renderer_backend.h"

#include <ngdevkit/registers.h>

void unsigned_palette_backend_load(u8 palette, const u16 *colors) {
    volatile u16 *target = &MMAP_PALBANK1[(u16)palette * U_SPRITE_PALETTE_COLOR_COUNT];
    for (u8 i = 0u; i < U_SPRITE_PALETTE_COLOR_COUNT; ++i) {
        target[i] = colors[i];
    }
}

void unsigned_palette_backend_set_backdrop_color(u16 color) {
    MMAP_PALBANK1[4095] = color;
}
