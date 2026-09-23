/**
 * @file background_backend.h
 * @brief Neo Geo scrolling-background backend boundary used by renderer orchestration.
 */

#ifndef UNSIGNED_SYSTEM_BACKGROUND_BACKEND_H
#define UNSIGNED_SYSTEM_BACKGROUND_BACKEND_H

#include "level/background/background.h"
#include "renderer/prepared_column.h"

/** Encode the default background shrink plus an optional viewport zoom effect. */
u16 unsigned_background_backend_encode_scb2(s16 zoom_offset);

/** Encode one complete prepared background SCB2/SCB3/SCB4 transform before VBlank. @pre `column` is valid. */
void unsigned_background_backend_encode_prepared_column(UPreparedColumn *column, s16 zoom_offset, s16 x, s16 y, u8 height_tiles);

/** Upload one background column; full=true uploads tiles and attributes, otherwise tiles only. */
void unsigned_background_backend_write_column(const UBackgroundLayerDefinition *definition, u8 physical_slot, u8 source_column, bool full);

#define U_BACKGROUND_TRANSFORM_DIRTY_SHRINK 0x01u
#define U_BACKGROUND_TRANSFORM_DIRTY_LAYOUT 0x02u
#define U_BACKGROUND_TRANSFORM_DIRTY_X 0x04u
#define U_BACKGROUND_TRANSFORM_DIRTY_Y 0x08u

/** Flush the hardware state of a chained background strip according to the supplied change flags. */
void unsigned_background_backend_flush_chained(const UBackgroundLayerDefinition *definition, u8 columns, u8 leftmost_slot, s16 base_x, s16 y, u16 scb2, u8 transform_dirty);

/** Stream per-column background transforms already encoded during CPU-side preparation. @pre `definition` and `prepared` are valid. */
void unsigned_background_backend_write_prepared_effect_columns(const UBackgroundLayerDefinition *definition, u8 columns, u8 leftmost_slot, const UPreparedColumn *prepared);

/** Hide a contiguous background sprite range. */
void unsigned_background_backend_hide_range(u16 first_sprite, u8 columns);

#endif
