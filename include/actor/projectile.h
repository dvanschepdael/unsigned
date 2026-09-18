/**
 * @file projectile.h
 * @brief Projectile actor wrapper.
 */

#ifndef UNSIGNED_ACTOR_PROJECTILE_H
#define UNSIGNED_ACTOR_PROJECTILE_H

#include "actor/actor.h"
#include "physics/trajectory.h"
#include "physics/trajectory_projection.h"

typedef struct UProjectile {
    UActor *actor;
    UTrajectory trajectory;
    UTrajectoryFunction trajectory_function;
    void *trajectory_context;
    const UTrajectoryProjection *projection;
    Vec2 projection_origin;
    Vec2 ground_position;
    Vec2 visual_position;
    u16 lifetime;
    bool collision_on_ground;
    bool bounce;
    bool collision_latched;
    bool expire_on_trajectory_end;
    UCallbackFunc on_hit;
    void *on_hit_args;
    UPhysicsCollisionChannel channel;
} UProjectile;

#endif
