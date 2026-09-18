/**
 * @file player_pool.c
 * @brief Implements fixed-capacity player pool binding, input routing and ticking.
 */

#include "actor/player_pool.h"

UPlayerPoolInstance *unsigned_player_pool_reserve(UPoolInstanceContainer *pool, UPlayer *player, u8 controller_index) {
    if (pool == NULL || player == NULL || player->character == NULL || controller_index >= U_INPUT_PLAYER_CAPACITY) {
        return NULL;
    }

    UPlayerPoolInstance *instance = unsigned_pool_reserve(pool);

    if (instance == NULL) {
        return NULL;
    }

    player->controller_index = controller_index;
    player->character->actor.active = true;
    instance->args = player;
    instance->elapsed = 0;

    return instance;
}

void unsigned_player_pool_release(UPoolInstanceContainer *pool, UAbilityPool *abilities, UPlayerPoolInstance *instance) {
    if (!unsigned_pool_owns(pool, instance) || !instance->active) {
        return;
    }

    UPlayer *player = instance->args;
    if (player != NULL) {
        unsigned_player_destroy(player, abilities);
    }

    unsigned_pool_release(pool, instance);
}

void unsigned_player_pool_tick(UPoolInstanceContainer *pool, UInputManager *input, UAbilityPool *abilities) {
    if (pool == NULL || pool->count == 0u) {
        return;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UPlayerPoolInstance *instance = &pool->instances[i];

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        UPlayer *player = instance->args;
        if (input != NULL && player->controller_index < input->player_count) {
            unsigned_player_tick_input(player, &input->players[player->controller_index], abilities);
        }
        unsigned_player_tick(player);
    }
}
