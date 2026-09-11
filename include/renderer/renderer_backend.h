/**
 * @file renderer_backend.h
 * @brief Boundary between renderer orchestration and the active video backend.
 *
 * @details
 * Renderer policy decides what must be drawn; concrete backends own hardware encoding and writes.
 */

#ifndef UNSIGNED_RENDERER_BACKEND_H
#define UNSIGNED_RENDERER_BACKEND_H

#include "display/sprite/sprite.h"
#include "display/viewport/viewport.h"
#include "level/background/background.h"

/** Begin a renderer batch. VRAMMOD writes may be cached until the matching end call. */
void unsigned_renderer_backend_begin(void);

/** End the current renderer batch and invalidate backend-side write caches. */
void unsigned_renderer_backend_end(void);

/** Encode Neo Geo SCB2 shrink values for a sprite plus an optional viewport zoom effect. */
u16 unsigned_sprite_backend_encode_scb2(const USpriteRenderState *render, s16 zoom_offset);

/** Flush sprite tile/attribute data selected by U_SPRITE_RENDER_DIRTY_GRAPHICS bits. */
void unsigned_sprite_backend_flush_graphics(const USprite *sprite, const UFrame *frame, u8 dirty);

/** Flush the changed SCB2/SCB3/SCB4 values for a normally chained sprite. */
void unsigned_sprite_backend_flush_chained_transform(const USprite *sprite, s16 screen_x, s16 screen_y, u16 scb2, u8 dirty);

/** Write per-column position/zoom values used by non-uniform viewport effects. */
void unsigned_sprite_backend_write_effect_positions(const USprite *sprite, const UViewport *viewport, s16 screen_x, s16 screen_y);

/** Clear SCB3 for a contiguous sprite range, making every sprite in the range invisible. */
void unsigned_sprite_backend_clear_range(u16 first_sprite, u16 sprite_count);

/** Hide only the driver of an already-owned chained sprite range. */
void unsigned_sprite_backend_hide_chain(const USprite *sprite);

/** Encode the default background shrink plus an optional viewport zoom effect. */
u16 unsigned_background_backend_encode_scb2(s16 zoom_offset);

/** Upload one background column; full=true uploads tiles and attributes, otherwise tiles only. */
void unsigned_background_backend_write_column(const UBackgroundLayerDefinition *definition, u8 physical_slot, u8 source_column, bool full);

/** Flush the hardware state of a chained background strip according to the supplied change flags. */
void unsigned_background_backend_flush_chained(const UBackgroundLayerDefinition *definition, u8 columns, u8 leftmost_slot, s16 base_x, s16 y, u16 scb2, bool shrink_changed, bool layout_changed, bool x_changed, bool y_changed);

/** Write per-column SCB2/SCB3/SCB4 values for a non-uniform background viewport effect. */
void unsigned_background_backend_write_effect_positions(const UBackgroundLayer *layer, const UViewport *viewport, u8 columns, u8 leftmost_slot, s16 base_x, s16 y);

/** Hide a contiguous background sprite range. */
void unsigned_background_backend_hide_range(u16 first_sprite, u8 columns);

/** Copy a complete 16-color palette into Neo Geo palette bank 1. */
void unsigned_palette_backend_load(u8 palette, const u16 *colors);

/** Update the global backdrop color in Neo Geo palette bank 1. */
void unsigned_palette_backend_set_backdrop_color(u16 color);

#endif
