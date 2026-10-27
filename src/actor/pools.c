/**
 * @file pools.c
 * @brief Implements aggregate actor pools owned by a game instance.
 */

#include "actor/pools.h"

void unsigned_actor_pools_init(UActorPoolSet *pools, const UActorPoolSetConfig *config) {
    unsigned_pool_init(&pools->players, config->players, config->player_capacity);
    unsigned_pool_init(&pools->npcs, config->npcs, config->npc_capacity);
    unsigned_pool_init(&pools->objects, config->objects, config->object_capacity);
    unsigned_pool_init(&pools->projectiles, config->projectiles, config->projectile_capacity);
}
