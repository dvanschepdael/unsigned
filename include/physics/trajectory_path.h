/**
 * @file trajectory_path.h
 * @brief Linear and arc trajectory path evaluation.
 */

#ifndef UNSIGNED_PHYSICS_TRAJECTORY_PATH_H
#define UNSIGNED_PHYSICS_TRAJECTORY_PATH_H

#include "physics/trajectory.h"

/**
 * @brief Configures a constant-velocity line from start to target over the requested frame count.
 *
 * @param trajectory Trajectory runtime state to configure.
 * @param start Starting x/depth/height position.
 * @param target Target position reached at the end of the path.
 * @param duration Path duration in frames; must be non-zero.
 * @pre `trajectory` is valid, `duration > 0`, and each derived fixed-point axis velocity fits in `s16`.
 */
void unsigned_physics_trajectory_set_line(UTrajectory *trajectory, UTrajectoryPoint start, UTrajectoryPoint target, u8 duration);

/**
 * @brief Configures a trajectory from start to target with a vertical arc above the linear path.
 *
 * @param trajectory Trajectory runtime state to configure.
 * @param start Starting x/depth/height position.
 * @param target Target position reached at the end of the path.
 * @param arc_height Arc strength used to derive vertical acceleration; must be non-zero.
 * @param duration Path duration in frames; must be non-zero.
 * @pre `trajectory` is valid, `duration > 0`, `arc_height > 0`, and derived velocity/acceleration values fit in `s16`.
 */
void unsigned_physics_trajectory_set_arc(UTrajectory *trajectory, UTrajectoryPoint start, UTrajectoryPoint target, u16 arc_height, u8 duration);

/**
 * @brief Evaluates the built-in linear path callback for the trajectory current elapsed frame.
 *
 * @param trajectory Trajectory being evaluated.
 * @param context Unused callback context; accepted to match UTrajectoryFunction.
 */
void unsigned_physics_trajectory_linear(UTrajectory *trajectory, void *context);

/**
 * @brief Integrates one built-in arc step and stops the trajectory when it falls back to height zero.
 *
 * @param trajectory Non-NULL arc trajectory configured by unsigned_physics_trajectory_set_arc().
 * @param context Unused callback context; accepted to match UTrajectoryFunction.
 */
void unsigned_physics_trajectory_arc(UTrajectory *trajectory, void *context);

#endif
