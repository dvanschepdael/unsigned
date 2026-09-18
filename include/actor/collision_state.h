/**
 * @file collision_state.h
 * @brief Per-actor transformed collision state.
 */

#ifndef UNSIGNED_ACTOR_COLLISION_STATE_H
#define UNSIGNED_ACTOR_COLLISION_STATE_H

#include "core/tlss/tlss.h"
#include "physics/collision.h"

typedef struct UActorCollisionState {
    UCollisionBox hitbox;
    UCollisionBox hurtbox;
    UPhysicsCollisionChannel hitbox_channel;
    UPhysicsCollisionChannel hurtbox_channel;
    UTLSSNode tlss_collision;
    bool hitbox_active;
    bool hurtbox_active;
    bool resolve_immediately;
} UActorCollisionState;

/**
 * @brief Clears transformed actor collision boxes and invalidates frame-local collision state.
 *
 * @param collision Collision state/configuration associated with the actor or manager.
 */
void unsigned_actor_collision_state_reset(UActorCollisionState *collision);

#endif
