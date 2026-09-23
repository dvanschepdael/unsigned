/**
 * @file player_pool.h
 * @brief Fixed-capacity player pool binding, input routing and ticking.
 */

#ifndef UNSIGNED_ACTOR_PLAYER_POOL_H
#define UNSIGNED_ACTOR_PLAYER_POOL_H

#include "actor/player.h"
#include "core/pool/pool.h"
#include "gameplay/ability_pool.h"

typedef UPoolInstance UPlayerPoolInstance;

/**
 * @brief Reserves an inactive slot from the player pool and binds it to the supplied runtime data.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param player Player runtime whose character/input state is read or updated.
 * @param controller_index Zero-based controller index.
 * @return Reserved active player pool slot.
 * @pre The pool has free capacity.
 * @pre `pool`, `player`, `player->character` are valid and `controller_index < U_INPUT_PLAYER_CAPACITY`.
 */
UPlayerPoolInstance *unsigned_player_pool_reserve(UPoolInstanceContainer *pool, UPlayer *player, u8 controller_index);

/**
 * @brief Releases the selected player pool runtime slot for reuse.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param abilities Runtime ability pool used to activate or release abilities.
 * @param instance Runtime pool slot to inspect or release.
 * @pre `pool`, `abilities`, `instance` and `instance->args` are valid.
 */
void unsigned_player_pool_release(UPoolInstanceContainer *pool, UAbilityPool *abilities, UPlayerPoolInstance *instance);

/**
 * @brief Advances the player pool by one scheduled engine frame.
 *
 * @param pool Fixed-capacity pool that owns the runtime slot.
 * @param input Input snapshot sampled or consumed by this API.
 * @param abilities Runtime ability pool used to activate or release abilities.
 * @pre `pool`, `input` and `abilities` are valid; every active slot owns a valid player.
 */
void unsigned_player_pool_tick(UPoolInstanceContainer *pool, UInputManager *input, UAbilityPool *abilities);

#endif
