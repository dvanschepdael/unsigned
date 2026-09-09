/**
 * @file game_internal.h
 * @brief Composition-root validation and derived-capacity helpers.
 *
 * These functions are internal because callers should provide a `UGameInstanceConfig` and let
 * `unsigned_game_instance_init()` enforce the complete set of capacity/storage invariants.
 */

#ifndef UNSIGNED_GAME_INTERNAL_H
#define UNSIGNED_GAME_INTERNAL_H

#include "game/game.h"

/** Build the collision topology derived from the configured actor-pool capacities. */
UCollisionManagerConfig game_collision_config_build(u8 players, u8 npcs, u8 objects, u8 projectiles);

/** Validate a complete game-instance configuration and return its derived runtime capacities. */
bool game_instance_config_validate(const UGameInstanceConfig *config, u8 *actor_capacity, UCollisionManagerConfig *collision_config);

#endif
