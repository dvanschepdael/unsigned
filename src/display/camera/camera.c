/**
 * @file camera.c
 * @brief Implements bounded integer camera movement.
 */

#include "display/camera/camera.h"

#include "core/math/math.h"

bool unsigned_camera_bounds_valid(const UCameraBounds *bounds) {
    return bounds != NULL && bounds->min_x <= bounds->max_x && bounds->min_y <= bounds->max_y;
}

void unsigned_camera_init(UCamera *camera, s16 x, s16 y) {
    if (camera == NULL) {
        return;
    }

    *camera = (UCamera){
        .x = x,
        .y = y,
        .previous_x = x,
        .previous_y = y,
    };
}

void unsigned_camera_begin_frame(UCamera *camera) {
    if (camera == NULL) {
        return;
    }

    camera->previous_x = camera->x;
    camera->previous_y = camera->y;
}

bool unsigned_camera_set_bounds(UCamera *camera, const UCameraBounds *bounds) {
    if (camera == NULL || !unsigned_camera_bounds_valid(bounds)) {
        return false;
    }

    camera->bounds = *bounds;
    camera->bounds_enabled = true;
    camera->x = unsigned_math_clamp_s16(camera->x, bounds->min_x, bounds->max_x);
    camera->y = unsigned_math_clamp_s16(camera->y, bounds->min_y, bounds->max_y);
    camera->previous_x = unsigned_math_clamp_s16(camera->previous_x, bounds->min_x, bounds->max_x);
    camera->previous_y = unsigned_math_clamp_s16(camera->previous_y, bounds->min_y, bounds->max_y);
    return true;
}

void unsigned_camera_clear_bounds(UCamera *camera) {
    if (camera != NULL) {
        camera->bounds_enabled = false;
    }
}

void unsigned_camera_set_position(UCamera *camera, s16 x, s16 y) {
    if (camera == NULL) {
        return;
    }

    if (camera->bounds_enabled) {
        x = unsigned_math_clamp_s16(x, camera->bounds.min_x, camera->bounds.max_x);
        y = unsigned_math_clamp_s16(y, camera->bounds.min_y, camera->bounds.max_y);
    }

    camera->x = x;
    camera->y = y;
}

s32 unsigned_camera_delta_x(const UCamera *camera) {
    return camera != NULL ? (s32)camera->x - camera->previous_x : 0;
}

s32 unsigned_camera_delta_y(const UCamera *camera) {
    return camera != NULL ? (s32)camera->y - camera->previous_y : 0;
}
