/**
 * @file collision_state.c
 * @brief Implements per-actor transformed collision state.
 */

#include "actor/collision_state.h"

void unsigned_actor_collision_state_reset(UActorCollisionState *collision) {
    if (collision == NULL) {
        return;
    }

    *collision = (UActorCollisionState){
        .hitbox_channel = U_COLLISION_CHANNEL_NONE,
        .hurtbox_channel = U_COLLISION_CHANNEL_NONE,
    };
}
