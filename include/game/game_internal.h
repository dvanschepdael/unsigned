/**
 * @file game_internal.h
 * @brief Composition-root capacity derivation helpers.
 *
 * These functions are internal because the public configuration expresses capacities once and the
 * composition root derives the collision/actor topology from that same source of truth.
 */

#ifndef UNSIGNED_GAME_INTERNAL_H
#define UNSIGNED_GAME_INTERNAL_H

#include "game/game.h"

/** Build the collision topology derived from the configured actor-pool capacities.
 * @pre The sum of all actor-pool capacities fits in u8. */
UCollisionManagerConfig game_collision_config_build(u8 players, u8 npcs, u8 objects, u8 projectiles);

#endif
