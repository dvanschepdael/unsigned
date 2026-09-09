/**
 * @file trajectory.h
 * @brief Frame-stepped trajectory state and movement.
 */

#ifndef UNSIGNED_PHYSICS_TRAJECTORY_H
#define UNSIGNED_PHYSICS_TRAJECTORY_H

#include "core/types.h"

#define U_TRAJECTORY_FIXED_ONE 256

typedef struct UTrajectoryPoint {
    s16 x;
    s16 depth;
    s16 height;
} UTrajectoryPoint;

typedef struct UTrajectoryVector {
    s16 x;
    s16 depth;
    s16 height;
} UTrajectoryVector;

typedef struct UTrajectory {
    s32 x;
    s32 depth;
    s32 height;
    UTrajectoryVector velocity;
    UTrajectoryVector acceleration;
    u8 elapsed;
    u8 duration;
    bool active;
} UTrajectory;

typedef void (*UTrajectoryFunction)(UTrajectory *trajectory, void *context);

/**
 * @brief Converts a whole-pixel value to the trajectory fixed-point representation.
 *
 * @param value Whole-pixel value to scale by `U_TRAJECTORY_FIXED_ONE`.
 * @return Fixed-point trajectory value.
 */
s32 unsigned_physics_trajectory_fixed(s32 value);
/**
 * @brief Initializes and activates a frame-stepped trajectory from position, velocity, acceleration and duration.
 *
 * @param trajectory Non-NULL trajectory runtime state to initialize.
 * @param position Initial x/depth/height position in pixels.
 * @param velocity Initial per-frame x/depth/height velocity in fixed-point units.
 * @param acceleration Per-frame fixed-point acceleration added to velocity.
 * @param duration Number of ticks before automatic deactivation; zero means no duration-based stop.
 */
void unsigned_physics_trajectory_set(UTrajectory *trajectory, UTrajectoryPoint position, UTrajectoryVector velocity, UTrajectoryVector acceleration, u8 duration);

/**
 * @brief Advances an active trajectory by one tick through the supplied path function.
 *
 * @param trajectory Trajectory runtime state to advance.
 * @param function Required path/evaluation callback invoked before elapsed is incremented.
 * @param context Opaque caller context passed to function.
 * @return true when the trajectory remains active after the tick; false for invalid/inactive input or when the callback/duration completes it.
 */
bool unsigned_physics_trajectory_tick(UTrajectory *trajectory, UTrajectoryFunction function, void *context);

/**
 * @brief Integrates one trajectory step by applying velocity to position and acceleration to velocity.
 *
 * @param trajectory Non-NULL trajectory runtime state to integrate.
 */
void unsigned_physics_trajectory_move(UTrajectory *trajectory);

/**
 * @brief Returns the current fixed-point trajectory position converted to saturated whole-pixel coordinates.
 *
 * @param trajectory Non-NULL trajectory runtime state to query.
 * @return Current x/depth/height position in pixels.
 */
UTrajectoryPoint unsigned_physics_trajectory_position(const UTrajectory *trajectory);

#endif
