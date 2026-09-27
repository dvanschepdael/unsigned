/**
 * @file actor_index.h
 * @brief Lookup index from registered collision boxes back to actors.
 */

#ifndef UNSIGNED_COLLISION_ACTOR_INDEX_H
#define UNSIGNED_COLLISION_ACTOR_INDEX_H

#include "physics/collision.h"

struct UActorContainer;

typedef struct UCollisionActorIndex {
    const struct UActorContainer *actors;
    /** Smallest/largest actor indices that can currently source an attack hitbox. */
    u8 hitbox_first;
    u8 hitbox_end;
    UPhysicsCollisionMask hitbox_channels;
    UPhysicsCollisionMask hurtbox_channels;
    UPhysicsCollisionMask covered_channels;
    s32 hitbox_min_top_offset;
    s32 hitbox_max_bottom_offset;
    s32 hurtbox_min_top_offset;
    s32 hurtbox_max_bottom_offset;
} UCollisionActorIndex;

typedef struct UCollisionActorRange {
    u8 first;
    u8 end;
} UCollisionActorRange;

/**
 * @brief Builds derived actor/channel/Y-extent metadata used to prune gameplay collision target searches.
 *
 * @param index Output actor-index runtime state.
 * @param actors Current level actor view; retained by pointer for later range queries.
 * @param collisions Collision manager used to determine which channels are fully represented by actors.
 * @pre `index`, `actors`, `collisions`, and actor storage are valid.
 * @pre `actors->instances[0..count)` contains active actors sorted by ascending world Y.
 * @pre Active actor collision boxes satisfy the `UActorCollisionState` channel invariants.
 */
void unsigned_collision_actor_index_build(UCollisionActorIndex *index, const struct UActorContainer *actors, const UCollisionManager *collisions);

/** Return the half-open actor span containing every currently active actor hitbox. */
UCollisionActorRange unsigned_collision_actor_index_hitbox_range(const UCollisionActorIndex *index);

/**
 * @brief Returns the actor subrange using only vertical extents relevant to the requested channels.
 *
 * Separating hitbox and hurtbox extents keeps large attack boxes from widening ordinary hurtbox
 * searches while remaining conservative for channels that may contain both categories.
 */
UCollisionActorRange unsigned_collision_actor_index_range_mask(const UCollisionActorIndex *index, const UCollisionBox *box, UPhysicsCollisionMask target_mask);

#endif
