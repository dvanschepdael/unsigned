/**
 * @file fix.c
 * @brief Implements Neo Geo FIX-layer text helpers.
 */

#include "system/neogeo/fix.h"

#include <ngdevkit/bios-calls.h>
#include <ngdevkit/ng-fix.h>

/** Clear the complete hardware FIX layer through the BIOS helper. */
void unsigned_neo_geo_fix_clear(void) {
    bios_fix_clear();
}

/** Center one normal-height FIX text line using the requested palette. */
void unsigned_neo_geo_fix_center_text(u8 row, u8 palette, const char *text) {
    if (text != NULL) {
        ng_center_text(row, palette, text);
    }
}

/** Center one tall FIX text line using ngdevkit's two-row glyph layout. */
void unsigned_neo_geo_fix_center_text_tall(u8 row, u8 palette, const char *text) {
    if (text != NULL) {
        ng_center_text_tall(row, palette, text);
    }
}
