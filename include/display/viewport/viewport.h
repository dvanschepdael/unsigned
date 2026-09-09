/**
 * @file viewport.h
 * @brief Camera-backed viewport, culling rectangle and world/screen coordinate conversion.
 */

#ifndef UNSIGNED_DISPLAY_VIEWPORT_H
#define UNSIGNED_DISPLAY_VIEWPORT_H

#include "display/camera/camera.h"
#include "display/viewport/effect.h"

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

/**
 * @brief Replaces the screen-space viewport rectangle used by culling and coordinate conversion.
 *
 * @param viewport Viewport used for culling, coordinate conversion or effects.
 * @param x Screen-space X coordinate of the viewport origin.
 * @param y Screen-space Y coordinate of the viewport origin.
 * @param width Width of the configured rectangle or element.
 * @param height Height of the configured rectangle or element.
 */
void unsigned_viewport_set_rect(UViewport *viewport, s16 x, s16 y, u16 width, u16 height);

/**
 * @brief Rebinds the non-owning camera pointer used as the world-space origin.
 *
 * @param viewport Viewport used for culling, coordinate conversion or effects.
 * @param camera Camera supplying the viewport world-space origin.
 */
void unsigned_viewport_set_camera(UViewport *viewport, UCamera *camera);

/**
 * @brief Advances the viewport by one scheduled engine frame.
 *
 * @param viewport Viewport used for culling, coordinate conversion or effects.
 */
void unsigned_viewport_tick(UViewport *viewport);

/**
 * @brief Returns whether the viewport is valid.
 *
 * @param viewport Viewport used for culling, coordinate conversion or effects.
 * @return true when the viewport exists, has non-zero dimensions and references a camera; false otherwise.
 */
bool unsigned_viewport_is_valid(const UViewport *viewport);

/**
 * @brief Converts a world-space X coordinate to viewport screen space.
 *
 * @param viewport Viewport used for culling, coordinate conversion or effects.
 * @param world_x World-space horizontal coordinate to convert.
 * @return The converted screen-space X coordinate, or 0 when the viewport is invalid.
 */
s16 unsigned_viewport_world_to_screen_x(const UViewport *viewport, s16 world_x);

/**
 * @brief Converts a world-space Y coordinate to viewport screen space.
 *
 * @param viewport Viewport used for culling, coordinate conversion or effects.
 * @param world_y World-space vertical coordinate to convert.
 * @return The converted screen-space Y coordinate, or 0 when the viewport is invalid.
 */
s16 unsigned_viewport_world_to_screen_y(const UViewport *viewport, s16 world_y);

/**
 * @brief Converts a viewport screen-space X coordinate to world space.
 *
 * @param viewport Viewport used for culling, coordinate conversion or effects.
 * @param screen_x Viewport screen-space horizontal coordinate to convert.
 * @return The converted world-space X coordinate, or 0 when the viewport is invalid.
 */
s16 unsigned_viewport_screen_to_world_x(const UViewport *viewport, s16 screen_x);

/**
 * @brief Converts a viewport screen-space Y coordinate to world space.
 *
 * @param viewport Viewport used for culling, coordinate conversion or effects.
 * @param screen_y Viewport screen-space vertical coordinate to convert.
 * @return The converted world-space Y coordinate, or 0 when the viewport is invalid.
 */
s16 unsigned_viewport_screen_to_world_y(const UViewport *viewport, s16 screen_y);

#endif
