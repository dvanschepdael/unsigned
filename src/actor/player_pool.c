/**
 * @file player_pool.c
 * @brief Implements fixed-capacity player pool binding, input routing and ticking.
 */

#include "actor/player_pool.h"

UPlayerPoolInstance *unsigned_player_pool_reserve(UPoolInstanceContainer *pool, UPlayer *player, u8 controller_index) {
    UPlayerPoolInstance *instance = unsigned_pool_reserve(pool);

    player->controller_index = controller_index;
    instance->args = player;
    instance->elapsed = 0;

    return instance;
}

void unsigned_player_pool_release(UPoolInstanceContainer *pool, UAbilityPool *abilities, UPlayerPoolInstance *instance) {
    UPlayer *player = instance->args;
    unsigned_player_destroy(player, abilities);

    unsigned_pool_release(pool, instance);
}

void unsigned_player_pool_tick(UPoolInstanceContainer *pool, UInputManager *input, UAbilityPool *abilities) {
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool); ++i) {
        UPlayerPoolInstance *instance = &pool->instances[i];

        if (!instance->active) {
            continue;
        }
        UPlayer *player = instance->args;
        unsigned_player_tick_input(player, &input->players[player->controller_index], abilities);
        unsigned_actor_tick(&player->character->actor);
    }
}
