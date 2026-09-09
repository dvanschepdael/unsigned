/**
 * @file trajectory.c
 * @brief Implements frame-stepped trajectory state and movement.
 */

#include "physics/trajectory.h"

#include "core/math/math.h"
s32 unsigned_physics_trajectory_fixed(s32 value) {
    return value * (s32)U_TRAJECTORY_FIXED_ONE;
}

/** Converts the trajectory fixed-point accumulator to its integer pixel coordinate. */
static s16 trajectory_fixed_to_pixel(s32 value) {
    return unsigned_math_saturate_s16(value / U_TRAJECTORY_FIXED_ONE);
}

void unsigned_physics_trajectory_set(UTrajectory *trajectory, UTrajectoryPoint position, UTrajectoryVector velocity, UTrajectoryVector acceleration, u8 duration) {
    *trajectory = (UTrajectory){
        .x = unsigned_physics_trajectory_fixed(position.x),
        .depth = unsigned_physics_trajectory_fixed(position.depth),
        .height = unsigned_physics_trajectory_fixed(position.height),
        .velocity = velocity,
        .acceleration = acceleration,
        .duration = duration,
        .active = true,
    };
}

bool unsigned_physics_trajectory_tick(UTrajectory *trajectory, UTrajectoryFunction function, void *context) {
    if (trajectory == NULL || function == NULL || !trajectory->active) {
        return false;
    }

    function(trajectory, context);
    trajectory->elapsed++;

    if (trajectory->duration > 0 && trajectory->elapsed >= trajectory->duration) {
        trajectory->active = false;
    }

    return trajectory->active;
}

void unsigned_physics_trajectory_move(UTrajectory *trajectory) {
    trajectory->x += trajectory->velocity.x;
    trajectory->depth += trajectory->velocity.depth;
    trajectory->height += trajectory->velocity.height;
    trajectory->velocity.x += trajectory->acceleration.x;
    trajectory->velocity.depth += trajectory->acceleration.depth;
    trajectory->velocity.height += trajectory->acceleration.height;
}

UTrajectoryPoint unsigned_physics_trajectory_position(const UTrajectory *trajectory) {
    return (UTrajectoryPoint){
        .x = trajectory_fixed_to_pixel(trajectory->x),
        .depth = trajectory_fixed_to_pixel(trajectory->depth),
        .height = trajectory_fixed_to_pixel(trajectory->height),
    };
}
