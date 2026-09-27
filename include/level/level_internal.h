/**
 * @file level_internal.h
 * @brief Internal collaboration API for focused level modules.
 */

#ifndef UNSIGNED_LEVEL_INTERNAL_H
#define UNSIGNED_LEVEL_INTERNAL_H

#include "input/input.h"
#include "level/level.h"
#include "level/level_runtime.h"

/**
 * @brief Releases all active typed actors and resets the derived level-wide actor view.
 *
 * @param level Loaded level whose actor runtime is cleared.
 */
void unsigned_level_actor_clear(ULevel *level);

/**
 * @brief Rebuilds the level-wide active actor view from the four typed fixed-capacity pools.
 *
 * @param level Loaded level whose actor view is synchronized.
 * @pre The actor-view storage is sized for the combined capacities of the four configured actor pools.
 */
void unsigned_level_actor_sync_pools(ULevel *level);

/**
 * @brief Synchronizes the typed pools into the level actor view and updates stable depth order.
 *
 * This is frame orchestration, not collision work: rendering and collision both consume the same
 * already-sorted actor view after this phase.
 *
 * @param level Loaded level whose actor view is built.
 * @pre `level` owns initialized actor pools and actor-view storage sized for their combined capacity.
 */
void unsigned_level_actor_build_view(ULevel *level);

/**
 * @brief Ticks active players, NPCs, objects and projectiles in the level-defined actor update phase.
 *
 * @param level Loaded level to advance.
 * @param input Current input manager consumed by player updates.
 */
void unsigned_level_actor_tick(ULevel *level, UInputManager *input);

/**
 * @brief Updates NPC activity classes from the level camera and runs AI according to each NPC TLSS cadence.
 *
 * @param level Loaded level containing the NPC pool and initialized camera.
 */
void unsigned_level_ai_tick(ULevel *level);

/**
 * @brief Instantiates NPC and object declarations from a level definition into the configured fixed pools.
 *
 * @param level Loaded level whose actor pools receive spawned content.
 * @param definition Static level content declaration to instantiate.
 * @param context Opaque application context forwarded to spawn/initialization callbacks.
 * @pre Spawn declarations satisfy the content invariants documented by `ULevelDefinition`,
 *      including fixed-pool capacity requirements.
 */
void unsigned_level_spawn_load(ULevel *level, const ULevelDefinition *definition, void *context);

/**
 * @brief Registers persistent collision for static level objects after load.
 *
 * @param level Loaded level whose persistent collision layers are populated.
 * @pre Static collision storage covers the authored objects that publish persistent boxes.
 */
void unsigned_level_collision_build_static(ULevel *level);

/** Reset persistent/dynamic collision state owned by the loaded level. */
void unsigned_level_collision_clear(ULevel *level);

/**
 * @brief Runs one complete level collision phase: dynamic registration, actor indexing, projectile resolution and attack-hit detection.
 *
 * @param level Loaded level whose frame-local collision results are rebuilt.
 */
void unsigned_level_collision_detect(ULevel *level);

#endif
