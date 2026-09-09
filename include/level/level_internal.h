/**
 * @file level_internal.h
 * @brief Internal collaboration API for focused level modules.
 */

#ifndef UNSIGNED_LEVEL_INTERNAL_H
#define UNSIGNED_LEVEL_INTERNAL_H

#include "input/input.h"
#include "level/level.h"
#include "level/level_actor.h"
#include "level/level_runtime.h"

/**
 * @brief Releases all active player, NPC, object and projectile instances owned by the loaded level.
 *
 * @param level Loaded level whose typed actor pools are cleared.
 */
void level_actor_release_pools(ULevel *level);

/**
 * @brief Rebuilds the level-wide active actor view from the four typed fixed-capacity pools.
 *
 * @param level Loaded level whose actor view is synchronized.
 * @return true when pool pointers/runtime entries fit the configured actor-view capacity; false on invalid/missing pool data or capacity overflow.
 */
bool level_actor_sync_pools(ULevel *level);

/**
 * @brief Ticks active players, NPCs, objects and projectiles in the level-defined actor update phase.
 *
 * @param level Loaded level to advance.
 * @param input Current input manager consumed by player updates.
 */
void level_actor_tick(ULevel *level, UInputManager *input);

/**
 * @brief Updates NPC activity classes from viewport visibility and runs AI according to each NPC TLSS cadence.
 *
 * @param level Loaded level containing the NPC pool.
 * @param viewport Current viewport used for active/off-screen classification; invalid viewport falls back to active behavior.
 */
void level_ai_tick(ULevel *level, const UViewport *viewport);

/**
 * @brief Instantiates NPC and object declarations from a level definition into the configured fixed pools.
 *
 * @param level Loaded level whose actor pools receive spawned content.
 * @param definition Static level content declaration to instantiate.
 * @param context Opaque application context forwarded to spawn/initialization callbacks.
 * @return true when all declared content reserves and initializes successfully; false on invalid data or pool exhaustion.
 */
bool level_spawn_load(ULevel *level, const ULevelDefinition *definition, void *context);

/**
 * @brief Registers persistent collision for static level objects after load.
 *
 * @param level Loaded level whose persistent collision layers are populated.
 * @return true when every eligible static object registers completely; false for invalid level state or collision-layer capacity overflow.
 */
bool level_collision_build_static(ULevel *level);

/**
 * @brief Runs one complete level collision phase: dynamic registration, actor indexing, projectile resolution and attack-hit detection.
 *
 * @param level Loaded level whose frame-local collision results are rebuilt.
 */
void level_collision_detect(ULevel *level);

#endif
