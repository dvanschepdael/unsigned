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
    /** World-space origin used by rendering and collision transforms. */
    Vec2 position;
    /** Mutable animation/presentation state; the immutable definition is owned by content. */
    USprite sprite;
    /** Optional non-owning sprite drawn/allocated before the actor sprite (for example a character shadow). */
    USprite *underlay;
    /** Frame-local transformed hitbox/hurtbox state derived from the current animation frame. */
    UActorCollisionState collision;
} UActor;

/**
 * Non-owning level-wide view of active actors collected from the specialized pools.
 * Every entry in the `count` prefix is active; pool synchronization owns that invariant. `instances`
 * points to caller-owned pointer storage and may be reordered for depth/collision work.
 */
typedef struct UActorContainer {
    /** Number of actors in the active prefix of `instances`. */
    u8 count;
    UActor **instances;
    /** Increments only when membership or sorted draw order changes. */
    u16 layout_revision;
} UActorContainer;

/**
 * @brief Initializes an empty actor view over caller-owned pointer storage.
 * @pre `container` is valid and `instances` is sized for the actor pools composed into this view.
 */
void unsigned_actor_container_init(UActorContainer *container, UActor **instances);

/**
 * @brief Advances one actor owned by an active pool slot by one scheduled engine frame.
 * @pre `actor` is valid and belongs to an active actor-pool slot.
 */
void unsigned_actor_tick(UActor *actor);

/**
 * @brief Batches presentation-only sprite ticks for an actor owned by an active pool slot.
 * @pre `actor` is valid, belongs to an active actor-pool slot, and unsigned_sprite_can_batch_ticks(&actor->sprite) is true.
 */
void unsigned_actor_tick_batch(UActor *actor, u16 ticks);

/** Reset frame-derived actor state before its owning pool slot is released. */
void unsigned_actor_destroy(UActor *actor);

/**
 * @brief Sorts the level actor view using the requested stable draw policy.
 *
 * Sorting changes presentation/collision traversal order only; actor ownership stays in the
 * specialized pools. Empty and single-entry views are deliberately skipped.
 *
 * @pre `order < U_ACTOR_ORDER_COUNT`.
 */
void unsigned_actor_container_sort(UActorContainer *container, UActorSort order);

#endif
