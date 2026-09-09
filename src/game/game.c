/**
 * @file game.c
 * @brief Builds the complete game runtime from fixed caller-owned storage and drives its frame loop.
 */

#include "game/game.h"

#include "actor/pools.h"
#include "game/config.h"
#include "game/game_internal.h"

/** Configure fixed-capacity actor pools and the gameplay runtime that operates on them. */
static void game_instance_init_actor_runtime(UGameInstance *game, const UGameInstanceConfig *config, u8 actor_capacity) {
    game->capacity = (UGameInstanceCapacity){
        .players = config->player_capacity,
        .npcs = config->npc_capacity,
        .objects = config->object_capacity,
        .projectiles = config->projectile_capacity,
        .actors = actor_capacity,
    };

    game->actor_pools = (UActorPoolSet){
        .players = {
            .capacity = config->player_capacity,
            .instances = config->storage.players,
        },
        .npcs = {
            .capacity = config->npc_capacity,
            .instances = config->storage.npcs,
        },
        .objects = {
            .capacity = config->object_capacity,
            .instances = config->storage.objects,
        },
        .projectiles = {
            .capacity = config->projectile_capacity,
            .instances = config->storage.projectiles,
        },
    };

    unsigned_actor_pools_init(&game->actor_pools);
    unsigned_gameplay_runtime_init(&game->gameplay);
}

/** Wire caller-owned level/spawn/collision storage into the level runtime. */
static bool game_instance_init_level_runtime(UGameInstance *game, const UGameInstanceConfig *config, u8 actor_capacity, const UCollisionManagerConfig *collision_config) {
    const ULevelSpawnStorage spawn_storage = {
        .npcs = config->storage.npc_runtime,
        .npc_characters = config->storage.npc_character_runtime,
        .objects = config->storage.object_runtime,
        .npc_capacity = config->npc_capacity,
        .object_capacity = config->object_capacity,
    };
    const UCollisionManagerStorage collision_storage = {
        .layer_boxes = config->storage.collision_layer_boxes,
        .layer_box_capacity = config->storage.collision_layer_box_capacity,
        .query_hits = config->storage.collision_query_hits,
        .query_hit_capacity = config->storage.collision_query_hit_capacity,
    };

    return unsigned_level_init(&game->level, &game->actor_pools, &game->gameplay, &spawn_storage, collision_config, &collision_storage, config->storage.actors, actor_capacity) && unsigned_game_instance_set_tlss_config(game, &config->tlss);
}

/** Start either graph-driven level flow or the simpler direct-level mode. */
static bool game_instance_init_level_flow(UGameInstance *game, const UGameInstanceConfig *config) {
    if (config->level_graph != NULL) {
        return unsigned_level_manager_init(&game->level_manager, &game->level, config->level_graph, config->level_condition_context, config->level_context);
    }

    return unsigned_level_manager_init_direct(&game->level_manager, &game->level, config->initial_level, config->level_context);
}

bool unsigned_game_instance_init(UGameInstance *game, const UGameInstanceConfig *config) {
    u8 actor_capacity;
    UCollisionManagerConfig collision_config;

    if (game == NULL) {
        return false;
    }

    *game = (UGameInstance){ 0 };
    if (!game_instance_config_validate(config, &actor_capacity, &collision_config)) {
        return false;
    }

    unsigned_level_renderer_init(&game->renderer);
    game_instance_init_actor_runtime(game, config, actor_capacity);

    if (!game_instance_init_level_runtime(game, config, actor_capacity, &collision_config) || !unsigned_input_manager_init(&game->input, config->player_capacity)) {
        goto fail;
    }

    unsigned_audio_manager_init(&game->audio);
    if (!unsigned_audio_manager_set_catalog(&game->audio, config->audio_catalog) || !unsigned_timer_pool_init(&game->timers, config->refresh_rate)) {
        goto fail;
    }

    unsigned_viewport_init(&game->viewport, 0, 0, UNSIGNED_GAME_SCREEN_WIDTH, UNSIGNED_GAME_SCREEN_HEIGHT, &game->level.camera);

    if (!game_instance_init_level_flow(game, config)) {
        goto fail;
    }

    return true;

fail:
    unsigned_game_instance_destroy(game);
    return false;
}

bool unsigned_game_instance_set_tlss_config(UGameInstance *game, const UTLSSScaleConfig *tlss_config) {
    return game != NULL && unsigned_level_set_tlss_config(&game->level, tlss_config);
}

u8 unsigned_game_instance_player_capacity(const UGameInstance *game) {
    return game != NULL ? game->capacity.players : 0u;
}

u8 unsigned_game_instance_npc_capacity(const UGameInstance *game) {
    return game != NULL ? game->capacity.npcs : 0u;
}

u8 unsigned_game_instance_object_capacity(const UGameInstance *game) {
    return game != NULL ? game->capacity.objects : 0u;
}

u8 unsigned_game_instance_projectile_capacity(const UGameInstance *game) {
    return game != NULL ? game->capacity.projectiles : 0u;
}

u8 unsigned_game_instance_actor_capacity(const UGameInstance *game) {
    return game != NULL ? game->capacity.actors : 0u;
}

UInputManager *unsigned_game_instance_input(UGameInstance *game) {
    return game != NULL ? &game->input : NULL;
}

UTimerPool *unsigned_game_instance_timers(UGameInstance *game) {
    return game != NULL ? &game->timers : NULL;
}

const ULevelDefinition *unsigned_game_instance_current_level(const UGameInstance *game) {
    return game != NULL ? unsigned_level_manager_current(&game->level_manager) : NULL;
}

UPoolInstanceContainer *unsigned_game_instance_player_pool(UGameInstance *game) {
    return game != NULL ? &game->actor_pools.players : NULL;
}

UPoolInstanceContainer *unsigned_game_instance_npc_pool(UGameInstance *game) {
    return game != NULL ? &game->actor_pools.npcs : NULL;
}

UPoolInstanceContainer *unsigned_game_instance_object_pool(UGameInstance *game) {
    return game != NULL ? &game->actor_pools.objects : NULL;
}

UPoolInstanceContainer *unsigned_game_instance_projectile_pool(UGameInstance *game) {
    return game != NULL ? &game->actor_pools.projectiles : NULL;
}

void unsigned_game_instance_send_level_event(UGameInstance *game, UEvent event) {
    if (game != NULL) {
        unsigned_level_manager_send_event(&game->level_manager, event);
    }
}

bool unsigned_game_instance_set_level(UGameInstance *game, const ULevelDefinition *definition) {
    return game != NULL && unsigned_level_manager_set(&game->level_manager, definition);
}

void unsigned_game_instance_tick(UGameInstance *game) {
    if (game == NULL) {
        return;
    }

    unsigned_timer_pool_tick(&game->timers);

    if (unsigned_level_manager_tick(&game->level_manager)) {
        unsigned_level_tick_with_viewport(&game->level, &game->input, &game->viewport);
        unsigned_viewport_tick(&game->viewport);
    }
    unsigned_audio_manager_tick(&game->audio);
}

void unsigned_game_instance_render(UGameInstance *game) {
    if (game != NULL) {
        unsigned_level_renderer_render(&game->renderer, &game->level, &game->viewport);
    }
}

void unsigned_game_instance_destroy(UGameInstance *game) {
    if (game == NULL) {
        return;
    }

    unsigned_level_renderer_hide(&game->renderer);
    unsigned_level_manager_stop(&game->level_manager);
    unsigned_gameplay_runtime_clear(&game->gameplay);
    unsigned_timer_pool_clear(&game->timers);
    *game = (UGameInstance){ 0 };
}
