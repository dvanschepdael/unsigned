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

/** Projects one trajectory axis from world origin into ground/visual coordinates. */
static s16 unsigned_physics_trajectory_projection_axis(s16 origin, s32 projected) {
    projected /= U_TRAJECTORY_PROJECTION_ONE;
    return unsigned_math_saturate_s16((s32)origin + projected);
}

/** Adds the supplied entry to the trajectory projection. */
static s16 trajectory_projection_add(s16 origin, s32 offset) {
    return unsigned_math_saturate_s16((s32)origin + offset);
}

/** Projects a trajectory using the standard ground/depth plus visual-height convention. */
static bool trajectory_projection_standard(UTrajectoryPoint position, const UTrajectoryProjection *projection, Vec2 origin, Vec2 *ground_position, Vec2 *visual_position) {
    s32 ground_y;

    if (projection == &U_TRAJECTORY_PROJECTION_SIDE) {
        ground_position->x = trajectory_projection_add(origin.x, position.x);
        ground_position->y = origin.y;
        visual_position->x = ground_position->x;
        visual_position->y = trajectory_projection_add(origin.y, -(s32)position.height);
        return true;
    }

    if (projection == &U_TRAJECTORY_PROJECTION_FRONT) {
        ground_y = -(s32)position.depth / 2;
        ground_position->x = trajectory_projection_add(origin.x, position.x);
        ground_position->y = trajectory_projection_add(origin.y, ground_y);
        visual_position->x = ground_position->x;
        visual_position->y = trajectory_projection_add(origin.y, (-(s32)position.depth - (s32)position.height * 2) / 2);
        return true;
    }

    if (projection == &U_TRAJECTORY_PROJECTION_TOP) {
        ground_y = position.depth;
        ground_position->x = trajectory_projection_add(origin.x, position.x);
        ground_position->y = trajectory_projection_add(origin.y, ground_y);
        visual_position->x = ground_position->x;
        visual_position->y = trajectory_projection_add(origin.y, ((s32)position.depth * 2 - position.height) / 2);
        return true;
    }

    return false;
}

bool unsigned_physics_trajectory_project(const UTrajectory *trajectory, const UTrajectoryProjection *projection, Vec2 origin, Vec2 *ground_position, Vec2 *visual_position) {
    UTrajectoryPoint position;

    if (trajectory == NULL || projection == NULL || ground_position == NULL || visual_position == NULL) {
        return false;
    }

    position.x = unsigned_math_saturate_s16(trajectory->x / U_TRAJECTORY_FIXED_ONE);
    position.depth = unsigned_math_saturate_s16(trajectory->depth / U_TRAJECTORY_FIXED_ONE);
    position.height = unsigned_math_saturate_s16(trajectory->height / U_TRAJECTORY_FIXED_ONE);

    if (trajectory_projection_standard(position, projection, origin, ground_position, visual_position)) {
        return true;
    }

    s32 ground_x = (s32)position.x * projection->screen_x_from_x + (s32)position.depth * projection->screen_x_from_depth;
    s32 ground_y = (s32)position.x * projection->screen_y_from_x + (s32)position.depth * projection->screen_y_from_depth;
    s32 height_x = (s32)position.height * projection->screen_x_from_height;
    s32 height_y = (s32)position.height * projection->screen_y_from_height;

    ground_position->x = unsigned_physics_trajectory_projection_axis(origin.x, ground_x);
    ground_position->y = unsigned_physics_trajectory_projection_axis(origin.y, ground_y);
    visual_position->x = unsigned_physics_trajectory_projection_axis(origin.x, ground_x + height_x);
    visual_position->y = unsigned_physics_trajectory_projection_axis(origin.y, ground_y + height_y);

    return true;
}
