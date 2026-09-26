/**
 * @file fix.c
 * @brief Implements Neo Geo FIX-layer text and tile helpers.
 */

#include "system/fix.h"

#include <ngdevkit/bios-calls.h>
#include <ngdevkit/ng-fix.h>

/** Clear the complete hardware FIX layer through the BIOS helper. */
void unsigned_neo_geo_fix_clear(void) {
    bios_fix_clear();
}

/** Draw one normal-height FIX text line at an explicit tile position. */
void unsigned_neo_geo_fix_text(u8 column, u8 row, u8 palette, const char *text) {
    ng_text_args(column, row, palette, SROM_TXT_TILE_OFFSET, text);
}

/** Center one normal-height FIX text line using the requested palette. */
void unsigned_neo_geo_fix_center_text(u8 row, u8 palette, const char *text) {
    ng_center_text(row, palette, text);
}

/** Center one tall FIX text line using ngdevkit's two-row glyph layout. */
void unsigned_neo_geo_fix_center_text_tall(u8 row, u8 palette, const char *text) {
    ng_center_text_tall(row, palette, text);
}

void unsigned_neo_geo_fix_draw_tile_strip(u8 column, u8 row, u8 palette, u16 base_tile, const u8 *tile_offsets, u8 count) {
    if (count == 0u) {
        return;
    }

    u8 drawable = count;
    const u8 remaining_columns = (u8)(UNSIGNED_NEO_GEO_FIX_COLUMNS - column);
    if (drawable > remaining_columns) {
        drawable = remaining_columns;
    }

    char text[UNSIGNED_NEO_GEO_FIX_COLUMNS + 1u];
    for (u8 i = 0u; i < drawable; ++i) {
        text[i] = (char)(tile_offsets[i] + 1u);
    }
    text[drawable] = '\0';

    /* ng_text_args adds each non-zero byte to start_tile. Subtracting one makes byte 1 select base_tile. */
    ng_text_args(column, row, palette, (u16)(base_tile - 1u), text);
}
