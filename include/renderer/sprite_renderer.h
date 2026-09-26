/**
 * @file sprite_renderer.h
 * @brief Renders one logical sprite using one or more consecutive Neo Geo hardware sprites.
 */

#ifndef UNSIGNED_RENDERER_SPRITE_RENDERER_H
#define UNSIGNED_RENDERER_SPRITE_RENDERER_H

#include "display/sprite/sprite.h"
#include "display/viewport/viewport.h"
#include "renderer/sprite_column_plan.h"

/** Relocate to a new contiguous hardware range without clearing the old range; the owning batch clears only slots that become unused. */
void unsigned_sprite_renderer_relocate(USprite *sprite, u16 first_sprite);

/**
 * Break a pending relocated chain boundary before the batch performs heavier VRAM writes.
 * This is a no-op when the sprite did not move to a new hardware range.
 */
void unsigned_sprite_renderer_precommit_chain_boundary(USprite *sprite);

/** Same visibility test using camera bounds already computed by the owning batch renderer. */
bool unsigned_sprite_renderer_is_visible_in_bounds(const USprite *sprite, const UViewport *viewport, const UViewportWorldBounds *bounds, const Vec2 *position);

/**
 * Prepare one allocated sprite entirely on the CPU, freezing per-column effects into frame scratch.
 * @pre `sprite` is initialized and owns a current frame; `viewport`, `position` and `column_buffer` are valid.
 * @pre `column_buffer` has room for every column required by this sprite when a per-column effect is active.
 */
void unsigned_sprite_renderer_prepare_draw(USprite *sprite, const UViewport *viewport, const Vec2 *position, USpriteColumnPlanBuffer *column_buffer);

/**
 * @brief Commit a sprite whose visibility, transforms and optional per-column effects were prepared before VBlank.
 * @pre `sprite` is valid and `sprite->render.prepared.valid` is true.
 */
void unsigned_sprite_renderer_draw_prepared(USprite *sprite);

/** Drop software ownership during CPU-side layout without touching VRAM. */
void unsigned_sprite_renderer_unassign(USprite *sprite);

/**
 * @brief Hides a contiguous hardware sprite range by clearing its SCB3 position state.
 * @pre `[first_sprite, first_sprite + sprite_count)` is a usable Neo Geo sprite range.
 */
void unsigned_sprite_renderer_clear_range(u16 first_sprite, u16 sprite_count);

#endif
