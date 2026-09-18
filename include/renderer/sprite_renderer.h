/**
 * @file sprite_renderer.h
 * @brief Renders one logical sprite using one or more consecutive Neo Geo hardware sprites.
 */

#ifndef UNSIGNED_RENDERER_SPRITE_RENDERER_H
#define UNSIGNED_RENDERER_SPRITE_RENDERER_H

#include "display/sprite/sprite.h"
#include "display/viewport/viewport.h"

/** Relocate to a new contiguous hardware range without clearing the old range; the owning batch clears only slots that become unused. */
bool unsigned_sprite_renderer_relocate(USprite *sprite, u16 first_sprite);

/**
 * Break a pending relocated chain boundary before the batch performs heavier VRAM writes.
 * This is a no-op when the sprite did not move to a new hardware range.
 */
void unsigned_sprite_renderer_precommit_chain_boundary(USprite *sprite);

/** Test whether the sprite intersects the camera viewport, conservatively including effect bounds. */
bool unsigned_sprite_renderer_is_visible(const USprite *sprite, const UViewport *viewport, const Vec2 *position);

/** CPU-only preparation of viewport/effect transforms for a sprite already allocated by an actor batch. */
bool unsigned_sprite_renderer_prepare_draw(USprite *sprite, const UViewport *viewport, const Vec2 *position);

/** Add the prepared sprite's expected critical/high VRAM word cost to the supplied accumulators. */
void unsigned_sprite_renderer_estimate_prepared_vram_words(const USprite *sprite, u32 *critical_words, u32 *high_words);

/** Cull, upload dirty state and draw one sprite at its world position. */
void unsigned_sprite_renderer_draw(USprite *sprite, const UViewport *viewport, const Vec2 *position);

/** Draw a sprite whose visibility/layout has already been prepared by a batch actor pass. */
void unsigned_sprite_renderer_draw_prepared(USprite *sprite, const UViewport *viewport, const Vec2 *position);

/** Temporarily hide the sprite while preserving enough state for efficient reappearance. */
void unsigned_sprite_renderer_hide(USprite *sprite);

/** Drop software ownership during CPU-side layout without touching VRAM. */
void unsigned_sprite_renderer_unassign(USprite *sprite);

/** Release renderer ownership immediately and force complete reconstruction if allocated again. */
void unsigned_sprite_renderer_release(USprite *sprite);

/** Clear SCB3 for a validated contiguous hardware sprite range. */
void unsigned_sprite_renderer_clear_range(u16 first_sprite, u16 sprite_count);

#endif
