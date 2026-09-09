/**
 * @file pools.c
 * @brief Implements aggregate actor pools owned by a game instance.
 */

#include "actor/pools.h"

void unsigned_actor_pools_init(UActorPoolSet *pools) {
    if (pools == NULL) {
        return;
    }

    unsigned_pool_init(&pools->players);
    unsigned_pool_init(&pools->npcs);
    unsigned_pool_init(&pools->objects);
    unsigned_pool_init(&pools->projectiles);
}
