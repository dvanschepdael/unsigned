/**
 * @file sprite_renderer_internal.h
 * @brief Internal actor-batching hooks for the sprite renderer.
 *
 * These functions coordinate ownership reuse and cross-sprite driver batching. They are not part
 * of the general sprite-renderer surface and should remain confined to renderer internals.
 */
#ifndef UNSIGNED_RENDERER_SPRITE_RENDERER_INTERNAL_H
#define UNSIGNED_RENDERER_SPRITE_RENDERER_INTERNAL_H

#include "renderer/sprite_renderer.h"

/** Snapshot the current hardware range/content before a batch may reorder sprites. */
void unsigned_sprite_renderer_snapshot_layout(USprite *sprite);

/** Return true when a relocated sprite can reuse the previous occupant's SCB1 graphics verbatim. */
bool unsigned_sprite_renderer_can_reuse_previous_graphics(const USprite *sprite, const USprite *previous_owner);

/** Adopt equivalent SCB1 graphics already present in the current range, avoiding a redundant upload. */
void unsigned_sprite_renderer_reuse_previous_graphics(USprite *sprite);

/** Return pure driver-position dirty bits when a prepared sprite is safe for cross-sprite batching. */
u8 unsigned_sprite_renderer_prepared_driver_dirty(const USprite *sprite);

/** Publish one driver axis already written by a cross-sprite batch and consume preparation when complete. */
void unsigned_sprite_renderer_commit_batched_driver_axis(USprite *sprite, u8 dirty_axis);

#endif
