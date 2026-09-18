/**
 * @file trajectory_projection.h
 * @brief Trajectory projection helpers.
 */

#ifndef UNSIGNED_PHYSICS_TRAJECTORY_PROJECTION_H
#define UNSIGNED_PHYSICS_TRAJECTORY_PROJECTION_H

#include "physics/trajectory.h"

#define U_TRAJECTORY_PROJECTION_ONE 16

typedef struct UTrajectoryProjection {
    s8 screen_x_from_x;
    s8 screen_x_from_depth;
    s8 screen_x_from_height;
    s8 screen_y_from_x;
    s8 screen_y_from_depth;
    s8 screen_y_from_height;
} UTrajectoryProjection;

extern const UTrajectoryProjection U_TRAJECTORY_PROJECTION_SIDE;
extern const UTrajectoryProjection U_TRAJECTORY_PROJECTION_FRONT;
extern const UTrajectoryProjection U_TRAJECTORY_PROJECTION_TOP;

/**
 * @brief Projects trajectory x/depth/height into ground and visual screen positions.
 *
 * @param trajectory Trajectory whose current fixed-point position is projected.
 * @param projection Projection coefficients or one of the predefined side/front/top projections.
 * @param origin Screen/world reference point added to the projection.
 * @param ground_position Receives the projected ground position used for depth/gameplay placement.
 * @param visual_position Receives the projected visual position after height displacement.
 * @return true when all required pointers are valid and projection succeeds; false otherwise.
 */
bool unsigned_physics_trajectory_project(const UTrajectory *trajectory, const UTrajectoryProjection *projection, Vec2 origin, Vec2 *ground_position, Vec2 *visual_position);

#endif
