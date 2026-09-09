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
    u8 count;
    UPhysicsCollisionMask channels;
    UPhysicsCollisionMask hurtbox_channels;
    UPhysicsCollisionMask covered_channels;
    s32 min_top_offset;
    s32 max_bottom_offset;
    bool sorted;
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
 * @return true when required pointers/container bounds are valid; false otherwise.
 */
bool unsigned_collision_actor_index_build(UCollisionActorIndex *index, const struct UActorContainer *actors, const UCollisionManager *collisions);

/**
 * @brief Returns the actor subrange whose possible Y extents can overlap a query box.
 *
 * @param index Previously built actor index.
 * @param box Positive-area world-space collision box to query.
 * @return Half-open [first,end) actor range; {0,0} for invalid/empty inputs, or the full range when actor ordering is not sorted.
 */
UCollisionActorRange unsigned_collision_actor_index_range(const UCollisionActorIndex *index, const UCollisionBox *box);

#endif
