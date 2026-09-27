/**
 * @file palette_backend.h
 * @brief Neo Geo palette backend boundary used by renderer orchestration.
 */

#ifndef UNSIGNED_SYSTEM_PALETTE_BACKEND_H
#define UNSIGNED_SYSTEM_PALETTE_BACKEND_H

#include "core/types.h"

/** Copy a complete 16-color palette into Neo Geo palette bank 1. */
void unsigned_system_palette_load(u8 palette, const u16 *colors);

/** Update the global backdrop color in Neo Geo palette bank 1. */
void unsigned_system_palette_set_backdrop(u16 color);

#endif
