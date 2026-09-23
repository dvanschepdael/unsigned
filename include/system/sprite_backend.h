/**
 * @file sprite_backend.h
 * @brief Neo Geo sprite backend boundary used by renderer orchestration.
 */

#ifndef UNSIGNED_SYSTEM_SPRITE_BACKEND_H
#define UNSIGNED_SYSTEM_SPRITE_BACKEND_H

#include "display/sprite/sprite.h"
#include "renderer/prepared_column.h"

/** Encode Neo Geo SCB2 shrink values for a sprite plus an optional viewport zoom effect. */
/**
 * @brief Encodes the final SCB2 shrink word from prepared sprite/effect state.
 * @pre `render` and `effect` are valid prepared frame data.
 */
u16 unsigned_sprite_backend_encode_scb2(const USpriteRenderState *render, const UEffectSample *effect);

/** Encode one complete prepared SCB2/SCB3/SCB4 transform before the VBlank commit. @pre `column` and `render` are valid. */
void unsigned_sprite_backend_encode_prepared_column(UPreparedColumn *column, const USpriteRenderState *render, const UEffectSample *effect, s16 x, s16 y, u8 height_tiles, bool hidden);

/**
 * @brief Returns whether the destination range already contains reusable transparent SCB1 padding.
 * @pre `first_sprite..first_sprite + sprite->render.layout.sprite_count - 1` is a valid hardware range.
 */
bool unsigned_sprite_backend_padding_is_valid(const USprite *sprite, u16 first_sprite);

/**
 * @brief Invalidates padding ownership after another renderer writes the same hardware columns.
 * @pre `sprite_count > 0` and the range is contained in the Neo Geo hardware sprite range.
 */
void unsigned_sprite_backend_invalidate_padding_range(u16 first_sprite, u16 sprite_count);

/** Flush sprite tile/attribute data and optional unused-row padding selected by dirty bits. */
void unsigned_sprite_backend_flush_graphics(const USprite *sprite, const UFrame *frame, u8 dirty);

/** Flush the changed SCB2/SCB3/SCB4 values for a normally chained sprite. */
void unsigned_sprite_backend_flush_chained_transform(const USprite *sprite, s16 screen_x, s16 screen_y, u16 scb2, u8 dirty);

/**
 * Stream driver-only X/Y updates for same-width contiguous chained sprites.
 * `dirty_position` contains U_SPRITE_RENDER_DIRTY_X and/or U_SPRITE_RENDER_DIRTY_Y.
 * @pre `count > 0`; `sprites` contains `count` valid same-width sprites and the first sprite owns a non-empty hardware range.
 */
void unsigned_sprite_backend_write_driver_batch(USprite *const *sprites, u8 count, u8 dirty_position);

/**
 * @brief Breaks one hardware chain boundary by clearing only the destination driver SCB3 word.
 * @pre `hardware_sprite` is within the Neo Geo hardware sprite range.
 */
void unsigned_sprite_backend_break_chain(u16 hardware_sprite);

/** Write per-column transform values already frozen during CPU-side frame preparation. @pre `sprite`, its definition and `columns` are valid. */
void unsigned_sprite_backend_write_prepared_effect_positions(const USprite *sprite, const UPreparedColumn *columns);

/** Clear SCB3 for a contiguous sprite range, making every sprite in the range invisible. */
void unsigned_sprite_backend_clear_range(u16 first_sprite, u16 sprite_count);

/** Hide only the driver of an already-owned chained sprite range. */
void unsigned_sprite_backend_hide_chain(const USprite *sprite);

#endif
