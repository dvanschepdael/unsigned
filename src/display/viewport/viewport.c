/**
 * @file viewport.c
 * @brief Implements camera-backed viewport conversion and world-space culling.
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

s16 unsigned_viewport_world_to_screen_x(const UViewport *viewport, s32 world_x) {
    if (!unsigned_viewport_is_valid(viewport)) {
        return 0;
    }

    return (s16)((s32)viewport->x + world_x - viewport->camera->x);
}

s16 unsigned_viewport_world_to_screen_y(const UViewport *viewport, s32 world_y) {
    if (!unsigned_viewport_is_valid(viewport)) {
        return 0;
    }

    return (s16)((s32)viewport->y + world_y - viewport->camera->y);
}

s16 unsigned_viewport_screen_to_world_x(const UViewport *viewport, s16 screen_x) {
    if (!unsigned_viewport_is_valid(viewport)) {
        return 0;
    }

    return (s16)((s32)screen_x - viewport->x + viewport->camera->x);
}

s16 unsigned_viewport_screen_to_world_y(const UViewport *viewport, s16 screen_y) {
    if (!unsigned_viewport_is_valid(viewport)) {
        return 0;
    }

    return (s16)((s32)screen_y - viewport->y + viewport->camera->y);
}

bool unsigned_viewport_intersects_world_rect(const UViewport *viewport, s32 world_x, s32 world_y, s32 width, s32 height, s32 margin_x, s32 margin_y) {
    if (!unsigned_viewport_is_valid(viewport) || width <= 0 || height <= 0) {
        return false;
    }

    if (margin_x < 0) {
        margin_x = 0;
    }
    if (margin_y < 0) {
        margin_y = 0;
    }

    const s32 left = (s32)viewport->camera->x - margin_x;
    const s32 top = (s32)viewport->camera->y - margin_y;
    const s32 right = (s32)viewport->camera->x + viewport->width + margin_x;
    const s32 bottom = (s32)viewport->camera->y + viewport->height + margin_y;

    return world_x + width > left && world_x < right && world_y + height > top && world_y < bottom;
}
