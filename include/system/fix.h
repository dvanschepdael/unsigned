/**
 * @file fix.h
 * @brief Neo Geo FIX-layer text helpers.
 */

#ifndef UNSIGNED_SYSTEM_FIX_H
#define UNSIGNED_SYSTEM_FIX_H

#include "core/types.h"

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
 */
void unsigned_neo_geo_fix_center_text(u8 row, u8 palette, const char *text);
/**
 * @brief Centers the neo geo FIX text tall.
 *
 * @param row Row index to use.
 * @param palette Palette data to use.
 * @param text Text bytes rendered to the Neo Geo FIX layer.
 */
void unsigned_neo_geo_fix_center_text_tall(u8 row, u8 palette, const char *text);

#endif
