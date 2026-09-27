/**
 * @file trajectory_path.c
 * @brief Implements linear and arc trajectory path evaluation.
 */

#include "physics/trajectory_path.h"

/** Computes the fixed-point per-frame velocity required on one axis. */
static s16 trajectory_axis_velocity(s16 start, s16 target, u8 duration) {
    return (s16)(unsigned_physics_trajectory_fixed((s32)target - start) / duration);
}

void unsigned_physics_trajectory_set_line(UTrajectory *trajectory, UTrajectoryPoint start, UTrajectoryPoint target, u8 duration) {
    const UTrajectoryVector velocity = {
        .x = trajectory_axis_velocity(start.x, target.x, duration),
        .depth = trajectory_axis_velocity(start.depth, target.depth, duration),
        .height = trajectory_axis_velocity(start.height, target.height, duration),
    };
    unsigned_physics_trajectory_set(trajectory, start, velocity, (UTrajectoryVector){0}, duration);
}

void unsigned_physics_trajectory_set_arc(UTrajectory *trajectory, UTrajectoryPoint start, UTrajectoryPoint target, u16 arc_height, u8 duration) {
    const s32 gravity = -(unsigned_physics_trajectory_fixed(arc_height) * 8) / ((s32)duration * duration);
    const s32 vertical_velocity = (unsigned_physics_trajectory_fixed((s32)target.height - start.height) - gravity * duration * (duration - 1) / 2) / duration;
    const UTrajectoryVector velocity = {
        .x = trajectory_axis_velocity(start.x, target.x, duration),
        .depth = trajectory_axis_velocity(start.depth, target.depth, duration),
        .height = (s16)vertical_velocity,
    };

    unsigned_physics_trajectory_set(trajectory, start, velocity, (UTrajectoryVector){.height = (s16)gravity}, duration);
}

void unsigned_physics_trajectory_linear(UTrajectory *trajectory, void *context) {
    (void)context;
    unsigned_physics_trajectory_move(trajectory);
}

void unsigned_physics_trajectory_arc(UTrajectory *trajectory, void *context) {
    const s16 vertical_velocity = trajectory->velocity.height;
    (void)context;
    unsigned_physics_trajectory_move(trajectory);

    if (trajectory->height <= 0 && vertical_velocity < 0) {
        trajectory->height = 0;
        trajectory->velocity.height = 0;
        trajectory->active = false;
    }
}
