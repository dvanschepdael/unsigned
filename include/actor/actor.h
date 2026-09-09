/**
 * @file actor.h
 * @brief World-facing state shared by every renderable/collidable entity.
 *
 * `UActor` deliberately stops at presentation and collision state. Combat-specific data
 * (attributes, abilities and tags) lives in `UCharacter`, while player/NPC control logic
 * wraps a character. This keeps objects and projectiles from paying for character-only state.
 */

#ifndef UNSIGNED_ACTOR_H
#define UNSIGNED_ACTOR_H

#include "actor/actor_sort.h"
#include "actor/collision_state.h"
#include "core/tlss/tlss.h"
#include "display/sprite/sprite.h"

typedef struct UActor {
    /** Logical participation flag; pool activity and actor activity are separate concerns. */
    bool active;
    /** World-space origin used by rendering and collision transforms. */
    Vec2 position;
    /** Mutable animation/presentation state; the immutable definition is owned by content. */
    USprite sprite;
    /** Frame-local transformed hitbox/hurtbox state derived from the current animation frame. */
    UActorCollisionState collision;
} UActor;

/**
 * Non-owning level-wide view of actors collected from the specialized pools.
 * `instances` points to caller-owned pointer storage and may be reordered for depth/collision work.
 */
typedef struct UActorContainer {
    u8 count;
    u8 capacity;
    UActor **instances;
} UActorContainer;

/** Initialize an empty actor view over caller-owned pointer storage. */
void unsigned_actor_container_init(UActorContainer *container, UActor **instances, u8 capacity);

/** Advance one actor by one scheduled engine frame. */
void unsigned_actor_tick(UActor *actor);

/** Reset the actor runtime state and release its logical resources. */
void unsigned_actor_destroy(UActor *actor);

/** Sort active actor pointers using the requested draw/order policy. */
void unsigned_actor_container_sort(UActorContainer *container, UActorSort order);

#endif
