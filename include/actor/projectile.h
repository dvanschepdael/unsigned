/**
 * @file projectile.h
 * @brief Projectile actor wrapper.
 */

#ifndef UNSIGNED_ACTOR_PROJECTILE_H
#define UNSIGNED_ACTOR_PROJECTILE_H

#include "actor/actor.h"
#include "physics/trajectory.h"
#include "physics/trajectory_projection.h"

/**
 * Runtime projectile motion, presentation and collision policy.
 *
 * @invariant An active projectile uses `channel < U_COLLISION_CHANNEL_COUNT`.
 */
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
    /** Optional hit reaction; it may update projectile policy but must not release/reuse the owning pool slot. */
    UCallbackFunc on_hit;
    void *on_hit_args;
    UPhysicsCollisionChannel channel;
} UProjectile;

#endif
