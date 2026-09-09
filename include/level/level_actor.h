/**
 * @file level_actor.h
 * @brief Level-wide actor pool synchronization and ticking.
 */

#ifndef UNSIGNED_LEVEL_ACTOR_H
#define UNSIGNED_LEVEL_ACTOR_H

#include "core/pool/pool.h"

struct ULevel;

/**
 * @brief Returns the player pool owned by the loaded level runtime.
 *
 * @param level Loaded level whose actor runtime is synchronized, ticked or queried.
 * @return Mutable player pool owned by the level actor-pool set, or NULL when the level is not wired.
 */
UPoolInstanceContainer *unsigned_level_player_pool(struct ULevel *level);

/**
 * @brief Returns the NPC pool owned by the loaded level runtime.
 *
 * @param level Loaded level whose actor runtime is synchronized, ticked or queried.
 * @return Mutable NPC pool owned by the level actor-pool set, or NULL when the level is not wired.
 */
UPoolInstanceContainer *unsigned_level_npc_pool(struct ULevel *level);

/**
 * @brief Returns the world-object pool owned by the loaded level runtime.
 *
 * @param level Loaded level whose actor runtime is synchronized, ticked or queried.
 * @return Mutable object pool owned by the level actor-pool set, or NULL when the level is not wired.
 */
UPoolInstanceContainer *unsigned_level_object_pool(struct ULevel *level);

/**
 * @brief Returns the projectile pool owned by the loaded level runtime.
 *
 * @param level Loaded level whose actor runtime is synchronized, ticked or queried.
 * @return Mutable projectile pool owned by the level actor-pool set, or NULL when the level is not wired.
 */
UPoolInstanceContainer *unsigned_level_projectile_pool(struct ULevel *level);

#endif
