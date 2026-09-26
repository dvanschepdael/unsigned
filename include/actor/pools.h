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

typedef struct UActorPoolSetConfig {
    UPoolInstance *players;
    UPoolInstance *npcs;
    UPoolInstance *objects;
    UPoolInstance *projectiles;
    u8 player_capacity;
    u8 npc_capacity;
    u8 object_capacity;
    u8 projectile_capacity;
} UActorPoolSetConfig;

/**
 * @brief Initializes the actor pools to a valid empty runtime state.
 *
 * @param pools Aggregate actor pool set to initialize.
 * @param config Caller-owned storage and capacities for each actor class.
 */
void unsigned_actor_pools_init(UActorPoolSet *pools, const UActorPoolSetConfig *config);

#endif
