/**
 * @file viewport.c
 * @brief Implements camera-backed viewport and coordinate conversion.
 */

#include "display/viewport/viewport.h"

void unsigned_viewport_init(UViewport *viewport, s16 x, s16 y, u16 width, u16 height, UCamera *camera) {
    if (viewport == NULL) {
        return;
    }

    *viewport = (UViewport){
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .camera = camera,
    };
}

void unsigned_viewport_set_rect(UViewport *viewport, s16 x, s16 y, u16 width, u16 height) {
    if (viewport == NULL) {
        return;
    }

    viewport->x = x;
    viewport->y = y;
    viewport->width = width;
    viewport->height = height;
}

void unsigned_viewport_set_camera(UViewport *viewport, UCamera *camera) {
    if (viewport != NULL) {
        viewport->camera = camera;
    }
}

void unsigned_viewport_tick(UViewport *viewport) {
    if (viewport != NULL) {
        unsigned_effect_tick(&viewport->effect);
    }
}

bool unsigned_viewport_is_valid(const UViewport *viewport) {
    return viewport != NULL && viewport->camera != NULL && viewport->width > 0u && viewport->height > 0u;
}

s16 unsigned_viewport_world_to_screen_x(const UViewport *viewport, s16 world_x) {
    if (!unsigned_viewport_is_valid(viewport)) {
        return 0;
    }

    return (s16)(viewport->x + world_x - viewport->camera->x);
}

s16 unsigned_viewport_world_to_screen_y(const UViewport *viewport, s16 world_y) {
    if (!unsigned_viewport_is_valid(viewport)) {
        return 0;
    }

    return (s16)(viewport->y + world_y - viewport->camera->y);
}

s16 unsigned_viewport_screen_to_world_x(const UViewport *viewport, s16 screen_x) {
    if (!unsigned_viewport_is_valid(viewport)) {
        return 0;
    }

    return (s16)(screen_x - viewport->x + viewport->camera->x);
}

s16 unsigned_viewport_screen_to_world_y(const UViewport *viewport, s16 screen_y) {
    if (!unsigned_viewport_is_valid(viewport)) {
        return 0;
    }

    return (s16)(screen_y - viewport->y + viewport->camera->y);
}
