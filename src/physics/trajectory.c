/**
 * @file trajectory.c
 * @brief Implements frame-stepped trajectory state and movement.
 */

#include "physics/trajectory.h"

#include "core/math/math.h"
s32 unsigned_physics_trajectory_fixed(s32 value) {
    return value * (s32)U_TRAJECTORY_FIXED_ONE;
}

s16 unsigned_physics_trajectory_parabola_height(u16 elapsed, u16 duration, s16 height) {
    if (elapsed == 0u || elapsed >= duration) {
        return 0;
    }

    const s32 t = elapsed;
    const s32 numerator = 4 * (s32)height * t * ((s32)duration - t);
    switch (duration) {
    case 8u:
        return unsigned_math_saturate_s16(numerator >> 6);
    case 16u:
        return unsigned_math_saturate_s16(numerator >> 8);
    case 32u:
        return unsigned_math_saturate_s16(numerator >> 10);
    case 64u:
        return unsigned_math_saturate_s16(numerator >> 12);
    case 128u:
        return unsigned_math_saturate_s16(numerator >> 14);
    default: {
        const s32 denominator = (s32)duration * duration;
        return unsigned_math_saturate_s16(numerator / denominator);
    }
    }
}

/** Converts the trajectory fixed-point accumulator to its integer pixel coordinate. */
static s16 trajectory_fixed_to_pixel(s32 value) {
    return unsigned_math_saturate_s16(unsigned_math_div_pow2_s32(value, 8u));
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

void unsigned_physics_trajectory_tick(UTrajectory *trajectory, UTrajectoryFunction function, void *context) {
    if (!trajectory->active) {
        return;
    }

    function(trajectory, context);
    trajectory->elapsed++;

    if (trajectory->duration > 0 && trajectory->elapsed >= trajectory->duration) {
        trajectory->active = false;
    }
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
