/**
 * @file fix.h
 * @brief Neo Geo FIX-layer text and tile helpers.
 */

#ifndef UNSIGNED_SYSTEM_FIX_H
#define UNSIGNED_SYSTEM_FIX_H

#include "core/types.h"

#define UNSIGNED_NEO_GEO_FIX_COLUMNS 40u
#define UNSIGNED_NEO_GEO_FIX_ROWS 32u

/**
 * @brief Clears the Neo Geo FIX state without freeing caller-owned storage.
 */
void unsigned_neo_geo_fix_clear(void);
/**
 * @brief Centers the neo geo FIX text.
 *
 * @param row Row index to use.
 * @param palette Palette data to use.
 * @param text Text bytes rendered to the Neo Geo FIX layer.
 * @pre `row < UNSIGNED_NEO_GEO_FIX_ROWS`, `palette < 16`, and `text` is valid.
 */
void unsigned_neo_geo_fix_center_text(u8 row, u8 palette, const char *text);
/**
 * @brief Centers the neo geo FIX text tall.
 *
 * @param row Row index to use.
 * @param palette Palette data to use.
 * @param text Text bytes rendered to the Neo Geo FIX layer.
 * @pre `row < UNSIGNED_NEO_GEO_FIX_ROWS`, `palette < 16`, and `text` is valid.
 */
void unsigned_neo_geo_fix_center_text_tall(u8 row, u8 palette, const char *text);

/**
 * @brief Draws a horizontal strip of FIX tiles selected as small offsets from one base tile.
 *
 * @details
 * Each `tile_offsets[i]` selects `base_tile + tile_offsets[i]`. The helper batches the whole strip
 * through ngdevkit's FIX text writer, so offsets must be in [0, 254] and `base_tile` must be at
 * least 1. The strip is clipped to the 40-column FIX map.
 *
 * @param column First FIX column.
 * @param row FIX row.
 * @param palette FIX palette index.
 * @param base_tile First tile in the caller's contiguous tile set.
 * @param tile_offsets Per-column tile offsets relative to `base_tile`.
 * @param count Number of tile offsets to draw. Zero performs no work.
 * @pre `column < UNSIGNED_NEO_GEO_FIX_COLUMNS`, `row < UNSIGNED_NEO_GEO_FIX_ROWS`, `base_tile > 0`,
 *      `palette < 16`, `tile_offsets` is valid when `count > 0`, and every consumed offset is in `[0, 254]`.
 */
void unsigned_neo_geo_fix_draw_tile_strip(u8 column, u8 row, u8 palette, u16 base_tile, const u8 *tile_offsets, u8 count);

#endif
