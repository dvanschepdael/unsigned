/**
 * @file camera.c
 * @brief Implements bounded integer camera movement.
 */

#include "display/camera/camera.h"

#include "core/math/math.h"

void unsigned_camera_init(UCamera *camera, s16 x, s16 y) {
    *camera = (UCamera){
        .x = x,
        .y = y,
        .previous_x = x,
        .previous_y = y,
    };
}

void unsigned_camera_begin_frame(UCamera *camera) {
    camera->previous_x = camera->x;
    camera->previous_y = camera->y;
}

void unsigned_camera_set_bounds(UCamera *camera, const UCameraBounds *bounds) {
    camera->bounds = *bounds;
    camera->bounds_enabled = true;
    camera->x = unsigned_math_clamp_s16(camera->x, bounds->min_x, bounds->max_x);
    camera->y = unsigned_math_clamp_s16(camera->y, bounds->min_y, bounds->max_y);
    camera->previous_x = unsigned_math_clamp_s16(camera->previous_x, bounds->min_x, bounds->max_x);
    camera->previous_y = unsigned_math_clamp_s16(camera->previous_y, bounds->min_y, bounds->max_y);
}

void unsigned_camera_clear_bounds(UCamera *camera) {
    camera->bounds_enabled = false;
}

void unsigned_camera_set_position(UCamera *camera, s16 x, s16 y) {
    if (camera->bounds_enabled) {
        x = unsigned_math_clamp_s16(x, camera->bounds.min_x, camera->bounds.max_x);
        y = unsigned_math_clamp_s16(y, camera->bounds.min_y, camera->bounds.max_y);
    }

    camera->x = x;
    camera->y = y;
}
