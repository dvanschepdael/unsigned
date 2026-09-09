/**
 * @file pools.h
 * @brief Aggregate actor pools owned by a game instance.
 */

#ifndef UNSIGNED_ACTOR_POOLS_H
#define UNSIGNED_ACTOR_POOLS_H

#include "core/pool/pool.h"

typedef struct UActorPoolSet {
    UPoolInstanceContainer players;
    UPoolInstanceContainer npcs;
    UPoolInstanceContainer objects;
    UPoolInstanceContainer projectiles;
} UActorPoolSet;

/**
 * @brief Initializes the actor pools to a valid empty runtime state.
 *
 * @param pools Aggregate actor pool set to initialize or inspect.
 */
void unsigned_actor_pools_init(UActorPoolSet *pools);

#endif
