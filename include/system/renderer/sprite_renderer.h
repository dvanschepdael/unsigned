/**
 * @file sprite_renderer.h
 * @brief Renders one logical sprite using one or more consecutive Neo Geo hardware sprites.
 */

#ifndef UNSIGNED_SYSTEM_RENDERER_SPRITE_RENDERER_H
#define UNSIGNED_SYSTEM_RENDERER_SPRITE_RENDERER_H

#include "display/sprite/sprite.h"
#include "display/viewport/viewport.h"

/** Relocate the sprite to a new contiguous hardware range and force a full rebuild on next draw. */
bool unsigned_sprite_renderer_relocate(USprite *sprite, u16 first_sprite);

/** Test whether the sprite intersects the camera viewport, conservatively including effect bounds. */
bool unsigned_sprite_renderer_is_visible(const USprite *sprite, const UViewport *viewport, const Vec2 *position);

/** Cull, upload dirty state and draw one sprite at its world position. */
void unsigned_sprite_renderer_draw(USprite *sprite, const UViewport *viewport, const Vec2 *position);

/** Draw a sprite whose visibility/layout has already been prepared by a batch actor pass. */
void unsigned_sprite_renderer_draw_prepared(USprite *sprite, const UViewport *viewport, const Vec2 *position);

/** Temporarily hide the sprite while preserving enough state for efficient reappearance. */
void unsigned_sprite_renderer_hide(USprite *sprite);

/** Release renderer ownership and force complete reconstruction if the sprite is allocated again. */
void unsigned_sprite_renderer_release(USprite *sprite);

/** Clear SCB3 for a validated contiguous hardware sprite range. */
void unsigned_sprite_renderer_clear_range(u16 first_sprite, u16 sprite_count);

#endif
