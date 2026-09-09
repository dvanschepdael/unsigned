/**
 * @file trajectory_path.c
 * @brief Implements linear and arc trajectory path evaluation.
 */

#include "physics/trajectory_path.h"

#include <limits.h>

/** Computes the constant per-frame velocity needed to reach the target on one axis. */
static bool trajectory_axis_velocity(s16 start, s16 target, u8 duration, s16 *velocity) {
    s32 value = unsigned_physics_trajectory_fixed((s32)target - start) / duration;

    if (value < INT16_MIN || value > INT16_MAX) {
        return false;
    }

    *velocity = (s16)value;
    return true;
}

bool unsigned_physics_trajectory_set_line(UTrajectory *trajectory, UTrajectoryPoint start, UTrajectoryPoint target, u8 duration) {
    UTrajectoryVector velocity;

    if (trajectory == NULL || duration == 0 || !trajectory_axis_velocity(start.x, target.x, duration, &velocity.x) || !trajectory_axis_velocity(start.depth, target.depth, duration, &velocity.depth) || !trajectory_axis_velocity(start.height, target.height, duration, &velocity.height)) {
        return false;
    }

    unsigned_physics_trajectory_set(trajectory, start, velocity, (UTrajectoryVector){ 0 }, duration);
    return true;
}

bool unsigned_physics_trajectory_set_arc(UTrajectory *trajectory, UTrajectoryPoint start, UTrajectoryPoint target, u16 arc_height, u8 duration) {
    UTrajectoryVector velocity;

    if (trajectory == NULL || duration == 0 || arc_height == 0) {
        return false;
    }

    s32 gravity = -(unsigned_physics_trajectory_fixed(arc_height) * 8) / ((s32)duration * duration);
    s32 vertical_velocity = (unsigned_physics_trajectory_fixed((s32)target.height - start.height) - gravity * duration * (duration - 1) / 2) / duration;

    if (gravity == 0 || gravity < INT16_MIN || gravity > INT16_MAX || vertical_velocity < INT16_MIN || vertical_velocity > INT16_MAX || !trajectory_axis_velocity(start.x, target.x, duration, &velocity.x) || !trajectory_axis_velocity(start.depth, target.depth, duration, &velocity.depth)) {
        return false;
    }

    velocity.height = (s16)vertical_velocity;
    unsigned_physics_trajectory_set(trajectory, start, velocity, (UTrajectoryVector){ .height = (s16)gravity }, duration);
    return true;
}

void unsigned_physics_trajectory_linear(UTrajectory *trajectory, void *context) {
    (void)context;
    unsigned_physics_trajectory_move(trajectory);
}

void unsigned_physics_trajectory_arc(UTrajectory *trajectory, void *context) {
    s16 vertical_velocity = trajectory->velocity.height;

    (void)context;
    unsigned_physics_trajectory_move(trajectory);

    if (trajectory->height <= 0 && vertical_velocity < 0) {
        trajectory->height = 0;
        trajectory->velocity.height = 0;
        trajectory->active = false;
    }
}
