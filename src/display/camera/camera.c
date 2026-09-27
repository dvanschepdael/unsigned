/**
 * @file camera.c
 * @brief Implements the unified camera view, bounded movement and world/screen projection.
 */

#include "display/camera/camera_internal.h"

#include "core/math/math.h"

void unsigned_camera_init(UCamera *camera, const UCameraConfig *config) {
    *camera = (UCamera){
        .x = config->position.x,
        .y = config->position.y,
        .previous_x = config->position.x,
        .previous_y = config->position.y,
        .screen_x = config->screen_x,
        .screen_y = config->screen_y,
        .width = config->width,
        .height = config->height,
    };
}

void unsigned_camera_set_screen_rect(UCamera *camera, s16 x, s16 y, u16 width, u16 height) {
    camera->screen_x = x;
    camera->screen_y = y;
    camera->width = width;
    camera->height = height;
}

void unsigned_camera_begin_frame(UCamera *camera) {
    camera->previous_x = camera->x;
    camera->previous_y = camera->y;
}

void unsigned_camera_tick(UCamera *camera) {
    unsigned_effect_tick(&camera->effect);
}

void unsigned_camera_set_limits(UCamera *camera, const UCameraLimits *limits) {
    camera->limits = *limits;
    camera->limits_enabled = true;
    camera->x = unsigned_math_clamp_s16(camera->x, limits->min_x, limits->max_x);
    camera->y = unsigned_math_clamp_s16(camera->y, limits->min_y, limits->max_y);
    camera->previous_x = unsigned_math_clamp_s16(camera->previous_x, limits->min_x, limits->max_x);
    camera->previous_y = unsigned_math_clamp_s16(camera->previous_y, limits->min_y, limits->max_y);
}

void unsigned_camera_clear_limits(UCamera *camera) {
    camera->limits_enabled = false;
}

void unsigned_camera_set_position(UCamera *camera, s16 x, s16 y) {
    if (camera->limits_enabled) {
        x = unsigned_math_clamp_s16(x, camera->limits.min_x, camera->limits.max_x);
        y = unsigned_math_clamp_s16(y, camera->limits.min_y, camera->limits.max_y);
    }

    camera->x = x;
    camera->y = y;
}

void unsigned_camera_world_bounds(const UCamera *camera, UCameraWorldBounds *bounds) {
    bounds->left = camera->x;
    bounds->top = camera->y;
    bounds->right = (s32)camera->x + camera->width;
    bounds->bottom = (s32)camera->y + camera->height;
}

bool unsigned_camera_world_bounds_intersects(const UCameraWorldBounds *bounds, s32 world_x, s32 world_y, s32 width, s32 height, s32 margin_x, s32 margin_y) {
    return unsigned_camera_world_bounds_intersects_unchecked(bounds, world_x, world_y, width, height, margin_x, margin_y);
}

s16 unsigned_camera_world_to_screen_x(const UCamera *camera, s32 world_x) {
    return unsigned_camera_world_to_screen_x_unchecked(camera, world_x);
}

s16 unsigned_camera_world_to_screen_y(const UCamera *camera, s32 world_y) {
    return unsigned_camera_world_to_screen_y_unchecked(camera, world_y);
}

s16 unsigned_camera_screen_to_world_x(const UCamera *camera, s16 screen_x) {
    return (s16)((s32)screen_x - camera->screen_x + camera->x);
}

s16 unsigned_camera_screen_to_world_y(const UCamera *camera, s16 screen_y) {
    return (s16)((s32)screen_y - camera->screen_y + camera->y);
}

bool unsigned_camera_intersects_world_rect(const UCamera *camera, s32 world_x, s32 world_y, s32 width, s32 height, s32 margin_x, s32 margin_y) {
    UCameraWorldBounds bounds;
    unsigned_camera_world_bounds(camera, &bounds);
    return unsigned_camera_world_bounds_intersects(&bounds, world_x, world_y, width, height, margin_x, margin_y);
}
