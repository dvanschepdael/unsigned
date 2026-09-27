/**
 * @file game.c
 * @brief Builds the complete game runtime from fixed caller-owned storage and drives its frame loop.
 */

#include "game/game.h"

#include "actor/pools.h"
#include "game/game_config.h"
#include "game/game_internal.h"

/** Configure fixed-capacity actor pools and the gameplay runtime that operates on them. */
static void game_instance_init_actor_runtime(UGameInstance *game, const UGameInstanceConfig *config) {
    unsigned_actor_pools_init(&game->actor_pools, &(UActorPoolSetConfig){
        .players = config->storage.players,
        .npcs = config->storage.npcs,
        .objects = config->storage.objects,
        .projectiles = config->storage.projectiles,
        .player_capacity = config->player_capacity,
        .npc_capacity = config->npc_capacity,
        .object_capacity = config->object_capacity,
        .projectile_capacity = config->projectile_capacity,
    });
    unsigned_gameplay_runtime_init(&game->gameplay);
}

/** Wire caller-owned level/spawn/collision storage into the level runtime. */
static void game_instance_init_level_runtime(UGameInstance *game, const UGameInstanceConfig *config, const UCollisionManagerConfig *collision_config) {
    const ULevelSpawnStorage spawn_storage = {
        .npcs = config->storage.npc_runtime,
        .npc_characters = config->storage.npc_character_runtime,
        .objects = config->storage.object_runtime,
    };
    unsigned_level_init(&game->level, &(ULevelRuntimeConfig){
                                          .actor_pools = &game->actor_pools,
                                          .gameplay = &game->gameplay,
                                          .spawn_storage = &spawn_storage,
                                          .collision_config = collision_config,
                                          .collision_layer_boxes = config->storage.collision_layer_boxes,
                                          .actor_instances = config->storage.actors,
                                          .camera =
                                              {
                                                  .position = {0, 0},
                                                  .screen_x = 0,
                                                  .screen_y = 0,
                                                  .width = UNSIGNED_GAME_SCREEN_WIDTH,
                                                  .height = UNSIGNED_GAME_SCREEN_HEIGHT,
                                              },
                                      });
    unsigned_level_set_tlss_config(&game->level, &config->tlss);
}

/** Start either graph-driven level flow or the simpler direct-level mode. */
static void game_instance_init_level_flow(UGameInstance *game, const UGameInstanceConfig *config) {
    if (config->level_graph != NULL) {
        unsigned_level_manager_init(&game->level_manager, &game->level, config->level_graph, config->level_condition_context, config->level_context);
        return;
    }

    unsigned_level_manager_init_direct(&game->level_manager, &game->level, config->initial_level, config->level_context);
}

void unsigned_game_init(UGameInstance *game, const UGameInstanceConfig *config) {
    const UCollisionManagerConfig collision_config = unsigned_game_capacity_build_collision(config->player_capacity, config->npc_capacity, config->object_capacity, config->projectile_capacity);

    *game = (UGameInstance){0};

    unsigned_renderer_level_init(&game->renderer);
    game_instance_init_actor_runtime(game, config);

    game_instance_init_level_runtime(game, config, &collision_config);
    unsigned_input_init(&game->input, config->player_capacity);

    unsigned_audio_init(&game->audio);
    unsigned_audio_event_set_definitions(&game->audio, config->audio_events);
    unsigned_timer_pool_init(&game->timers, config->refresh_rate);


    game_instance_init_level_flow(game, config);
}

void unsigned_game_set_level(UGameInstance *game, const ULevelDefinition *definition) {
    unsigned_level_manager_set(&game->level_manager, definition);
}

void unsigned_game_tick(UGameInstance *game) {
    unsigned_timer_pool_tick(&game->timers);

    unsigned_level_manager_tick(&game->level_manager);
    if (game->level_manager.status == U_LEVEL_MANAGER_ACTIVE) {
        unsigned_level_tick(&game->level, &game->input);
        unsigned_camera_tick(&game->level.camera);
    }
    unsigned_audio_tick(&game->audio);
}

void unsigned_game_render_build(UGameInstance *game) {
    unsigned_renderer_level_build(&game->renderer, &game->level);
}

void unsigned_game_commit_render(UGameInstance *game) {
    unsigned_renderer_level_commit(&game->renderer);
}

void unsigned_game_destroy(UGameInstance *game) {
    unsigned_renderer_level_hide(&game->renderer);
    unsigned_level_manager_stop(&game->level_manager);
    unsigned_gameplay_runtime_clear(&game->gameplay);
    unsigned_timer_pool_clear(&game->timers);
    *game = (UGameInstance){0};
}
