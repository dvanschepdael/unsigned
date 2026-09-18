/**
 * @file viewport.h
 * @brief Camera-backed viewport, culling rectangle and world/screen coordinate conversion.
 */

#ifndef UNSIGNED_DISPLAY_VIEWPORT_H
#define UNSIGNED_DISPLAY_VIEWPORT_H

#include "display/camera/camera.h"
#include "display/effect/effect.h"

typedef struct UViewport {
    s16 x;
    s16 y;
    u16 width;
    u16 height;
    UCamera *camera;
    UEffect effect;
} UViewport;

/**
 * @brief Initializes the viewport to a valid empty runtime state.
 *
 * @param viewport Viewport used for culling, coordinate conversion or effects.
 * @param x Screen-space X coordinate of the viewport origin.
 * @param y Screen-space Y coordinate of the viewport origin.
 * @param width Width of the configured rectangle or element.
 * @param height Height of the configured rectangle or element.
 * @param camera Camera supplying the viewport world-space origin.
 */
void unsigned_viewport_init(UViewport *viewport, s16 x, s16 y, u16 width, u16 height, UCamera *camera);

/** Replace the screen-space rectangle used by culling and coordinate conversion. */
void unsigned_viewport_set_rect(UViewport *viewport, s16 x, s16 y, u16 width, u16 height);

/** Rebind the non-owning camera pointer used as the world-space origin. */
void unsigned_viewport_set_camera(UViewport *viewport, UCamera *camera);

/** Advance viewport-owned presentation effects by one engine frame. */
void unsigned_viewport_tick(UViewport *viewport);

/** Return true when dimensions are non-zero and a camera is bound. */
bool unsigned_viewport_is_valid(const UViewport *viewport);

/** Convert one world-space X coordinate to screen space. */
s16 unsigned_viewport_world_to_screen_x(const UViewport *viewport, s32 world_x);

/** Convert one world-space Y coordinate to screen space. */
s16 unsigned_viewport_world_to_screen_y(const UViewport *viewport, s32 world_y);

/** Convert one screen-space X coordinate to world space. */
s16 unsigned_viewport_screen_to_world_x(const UViewport *viewport, s16 screen_x);

/** Convert one screen-space Y coordinate to world space. */
s16 unsigned_viewport_screen_to_world_y(const UViewport *viewport, s16 screen_y);

/**
 * Test an axis-aligned world rectangle against the camera rectangle expanded by margins.
 * Width/height must be positive. Margins are symmetric and may be zero.
 */
bool unsigned_viewport_intersects_world_rect(const UViewport *viewport, s32 world_x, s32 world_y, s32 width, s32 height, s32 margin_x, s32 margin_y);

#endif
