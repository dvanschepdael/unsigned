/**
 * @file limits.h
 * @brief Defines limits used by the sprite subsystem.
 */

#ifndef UNSIGNED_DISPLAY_SPRITE_LIMITS_H
#define UNSIGNED_DISPLAY_SPRITE_LIMITS_H

#include "core/types.h"

#define UNSIGNED_SPRITE_FIRST 1u
#define UNSIGNED_SPRITE_LAST 381u
#define UNSIGNED_SPRITE_MAX_HEIGHT_TILES 32u
/* Neo Geo sprite parser hard limit: at most 96 hardware sprite columns may be active on one raster line. */
#define UNSIGNED_SPRITE_MAX_PER_SCANLINE 96u

#endif
