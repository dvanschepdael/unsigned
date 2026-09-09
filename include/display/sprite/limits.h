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

/**
 * @brief Returns whether the sprite range is valid.
 *
 * @param first_sprite First Neo Geo hardware sprite index owned by the logical sprite/range.
 * @param sprite_count Number of consecutive Neo Geo hardware sprites in the range.
 * @return true when the non-empty contiguous range stays inside the hardware sprite indices reserved by the engine.
 */
static inline bool unsigned_sprite_range_is_valid(u16 first_sprite, u8 sprite_count) {
    u32 last_sprite;

    if (sprite_count == 0u || first_sprite < UNSIGNED_SPRITE_FIRST || first_sprite > UNSIGNED_SPRITE_LAST) {
        return false;
    }

    last_sprite = (u32)first_sprite + (u32)sprite_count - 1u;
    return last_sprite <= UNSIGNED_SPRITE_LAST;
}

/**
 * @brief Returns whether the sprite height is valid.
 *
 * @param height_tiles Sprite height in 16-pixel tiles, constrained by the Neo Geo hardware sprite limit.
 * @return true for heights 1..32 tiles, the Neo Geo hardware sprite-height range used by this renderer.
 */
static inline bool unsigned_sprite_height_is_valid(u8 height_tiles) {
    return height_tiles != 0u && height_tiles <= UNSIGNED_SPRITE_MAX_HEIGHT_TILES;
}

#endif
