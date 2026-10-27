/**
 * @file trajectory_projection.c
 * @brief Implements trajectory projection helpers.
 */

#include "physics/trajectory_projection.h"

#include "core/math/math.h"

const UTrajectoryProjection U_TRAJECTORY_PROJECTION_SIDE = {
    .screen_x_from_x = U_TRAJECTORY_PROJECTION_ONE,
    .screen_y_from_height = -U_TRAJECTORY_PROJECTION_ONE,
};

const UTrajectoryProjection U_TRAJECTORY_PROJECTION_FRONT = {
    .screen_x_from_x = U_TRAJECTORY_PROJECTION_ONE,
    .screen_y_from_depth = -(U_TRAJECTORY_PROJECTION_ONE / 2),
    .screen_y_from_height = -U_TRAJECTORY_PROJECTION_ONE,
};

const UTrajectoryProjection U_TRAJECTORY_PROJECTION_TOP = {
    .screen_x_from_x = U_TRAJECTORY_PROJECTION_ONE,
    .screen_y_from_depth = U_TRAJECTORY_PROJECTION_ONE,
    .screen_y_from_height = -(U_TRAJECTORY_PROJECTION_ONE / 2),
};

static s16 projection_axis(s16 origin, s32 projected) {
    return unsigned_math_saturate_s16((s32)origin + unsigned_math_div_pow2_s32(projected, 4u));
}

static s16 projection_add(s16 origin, s32 offset) {
    return unsigned_math_saturate_s16((s32)origin + offset);
}

void unsigned_physics_trajectory_project(const UTrajectory *trajectory, const UTrajectoryProjection *projection, Vec2 origin, Vec2 *ground_position, Vec2 *visual_position) {
    UTrajectoryPoint position = {
        .x = unsigned_math_saturate_s16(unsigned_math_div_pow2_s32(trajectory->x, 8u)),
        .height = unsigned_math_saturate_s16(unsigned_math_div_pow2_s32(trajectory->height, 8u)),
    };

    if (projection == &U_TRAJECTORY_PROJECTION_SIDE) {
        ground_position->x = projection_add(origin.x, position.x);
        ground_position->y = origin.y;
        visual_position->x = ground_position->x;
        visual_position->y = projection_add(origin.y, -(s32)position.height);
        return;
    }

    position.depth = unsigned_math_saturate_s16(unsigned_math_div_pow2_s32(trajectory->depth, 8u));
    if (projection == &U_TRAJECTORY_PROJECTION_FRONT) {
        const s32 ground_y = unsigned_math_div_pow2_s32(-(s32)position.depth, 1u);
        ground_position->x = projection_add(origin.x, position.x);
        ground_position->y = projection_add(origin.y, ground_y);
        visual_position->x = ground_position->x;
        visual_position->y = projection_add(origin.y, unsigned_math_div_pow2_s32(-(s32)position.depth - (s32)position.height * 2, 1u));
        return;
    }

    if (projection == &U_TRAJECTORY_PROJECTION_TOP) {
        ground_position->x = projection_add(origin.x, position.x);
        ground_position->y = projection_add(origin.y, position.depth);
        visual_position->x = ground_position->x;
        visual_position->y = projection_add(origin.y, unsigned_math_div_pow2_s32((s32)position.depth * 2 - position.height, 1u));
        return;
    }

    const s32 ground_x = (s32)position.x * projection->screen_x_from_x + (s32)position.depth * projection->screen_x_from_depth;
    const s32 ground_y = (s32)position.x * projection->screen_y_from_x + (s32)position.depth * projection->screen_y_from_depth;
    const s32 height_x = (s32)position.height * projection->screen_x_from_height;
    const s32 height_y = (s32)position.height * projection->screen_y_from_height;

    ground_position->x = projection_axis(origin.x, ground_x);
    ground_position->y = projection_axis(origin.y, ground_y);
    visual_position->x = projection_axis(origin.x, ground_x + height_x);
    visual_position->y = projection_axis(origin.y, ground_y + height_y);
}
