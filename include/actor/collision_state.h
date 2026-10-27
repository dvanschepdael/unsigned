/**
 * @file collision_state.h
 * @brief Per-actor transformed collision state.
 */

#ifndef UNSIGNED_ACTOR_COLLISION_STATE_H
#define UNSIGNED_ACTOR_COLLISION_STATE_H

#include "core/tlss/tlss.h"
#include "physics/collision.h"

/**
 * Frame-local collision materialized from the actor's current animation frame.
 *
 * @invariant `hitbox_channel == U_COLLISION_CHANNEL_NONE` means no active hitbox.
 * @invariant `hurtbox_channel == U_COLLISION_CHANNEL_NONE` means no active hurtbox.
 */
typedef struct UActorCollisionState {
    UCollisionBox hitbox;
    UCollisionBox hurtbox;
    /** Last frame/transform used to build the actor-owned world-space boxes; NULL means the cache is invalid. */
    const void *transform_frame;
    Vec2 transform_position;
    UPhysicsCollisionChannel hitbox_channel;
    UPhysicsCollisionChannel hurtbox_channel;
    UTLSSNode tlss_collision;
    u8 transform_flip_x;
    bool resolve_immediately;
} UActorCollisionState;

/**
 * @brief Clears transformed actor collision boxes and invalidates frame-local collision state.
 *
 * @param collision Collision state/configuration associated with the actor or manager.
 * @pre `collision` is valid.
 */
void unsigned_actor_collision_state_reset(UActorCollisionState *collision);

#endif
