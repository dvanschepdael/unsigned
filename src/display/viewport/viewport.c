/**
 * @file viewport.c
 * @brief Implements camera-backed viewport conversion and world-space culling.
 */

#include "display/viewport/viewport_internal.h"

void unsigned_viewport_init(UViewport *viewport, s16 x, s16 y, u16 width, u16 height, UCamera *camera) {
    *viewport = (UViewport){
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .camera = camera,
    };
}

void unsigned_viewport_set_rect(UViewport *viewport, s16 x, s16 y, u16 width, u16 height) {
    viewport->x = x;
    viewport->y = y;
    viewport->width = width;
    viewport->height = height;
}

void unsigned_viewport_set_camera(UViewport *viewport, UCamera *camera) {
    viewport->camera = camera;
}

void unsigned_viewport_tick(UViewport *viewport) {
    unsigned_effect_tick(&viewport->effect);
}

void unsigned_viewport_world_bounds(const UViewport *viewport, UViewportWorldBounds *bounds) {
    bounds->left = viewport->camera->x;
    bounds->top = viewport->camera->y;
    bounds->right = (s32)viewport->camera->x + viewport->width;
    bounds->bottom = (s32)viewport->camera->y + viewport->height;
}

bool unsigned_viewport_world_bounds_intersects(const UViewportWorldBounds *bounds, s32 world_x, s32 world_y, s32 width, s32 height, s32 margin_x, s32 margin_y) {
    return unsigned_viewport_world_bounds_intersects_unchecked(bounds, world_x, world_y, width, height, margin_x, margin_y);
}

s16 unsigned_viewport_world_to_screen_x(const UViewport *viewport, s32 world_x) {
    return unsigned_viewport_world_to_screen_x_unchecked(viewport, world_x);
}

s16 unsigned_viewport_world_to_screen_y(const UViewport *viewport, s32 world_y) {
    return unsigned_viewport_world_to_screen_y_unchecked(viewport, world_y);
}

s16 unsigned_viewport_screen_to_world_x(const UViewport *viewport, s16 screen_x) {
    return (s16)((s32)screen_x - viewport->x + viewport->camera->x);
}

s16 unsigned_viewport_screen_to_world_y(const UViewport *viewport, s16 screen_y) {
    return (s16)((s32)screen_y - viewport->y + viewport->camera->y);
}

bool unsigned_viewport_intersects_world_rect(const UViewport *viewport, s32 world_x, s32 world_y, s32 width, s32 height, s32 margin_x, s32 margin_y) {
    UViewportWorldBounds bounds;
    unsigned_viewport_world_bounds(viewport, &bounds);
    return unsigned_viewport_world_bounds_intersects(&bounds, world_x, world_y, width, height, margin_x, margin_y);
}
