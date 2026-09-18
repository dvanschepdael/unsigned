/**
 * @file game_validation.c
 * @brief Validates caller-owned storage and derived capacities before game initialization.
 */

#include "game/config.h"
#include "game/game_internal.h"

static bool game_instance_actor_storage_is_valid(const UGameInstanceConfig *config, u8 actor_capacity) {
    const UGameInstanceStorage *storage = &config->storage;

    if (storage->players == NULL || storage->actors == NULL || storage->actor_capacity < actor_capacity) {
        return false;
    }
    if (config->npc_capacity > 0u && (storage->npcs == NULL || storage->npc_runtime == NULL || storage->npc_character_runtime == NULL)) {
        return false;
    }
    if (config->object_capacity > 0u && (storage->objects == NULL || storage->object_runtime == NULL)) {
        return false;
    }
    if (config->projectile_capacity > 0u && storage->projectiles == NULL) {
        return false;
    }
    return true;
}

static bool game_instance_collision_storage_is_valid(const UGameInstanceConfig *config, const UCollisionManagerConfig *collision_config) {
    const UGameInstanceStorage *storage = &config->storage;
    const u16 layer_capacity = unsigned_physics_collision_layer_storage_capacity(collision_config);
    const u16 compile_time_layer_capacity = (u16)U_GAME_COLLISION_LAYER_STORAGE_CAPACITY(config->player_capacity, config->npc_capacity, config->object_capacity, config->projectile_capacity);

    /* Keep the C99 static-array formula synchronized with the runtime collision topology. */
    if (layer_capacity != compile_time_layer_capacity) {
        return false;
    }
    if (layer_capacity > 0u && storage->collision_layer_boxes == NULL) {
        return false;
    }
    if (storage->collision_layer_box_capacity < layer_capacity) {
        return false;
    }
    if (collision_config->query_hit_capacity > 0u && storage->collision_query_hits == NULL) {
        return false;
    }
    if (storage->collision_query_hit_capacity < collision_config->query_hit_capacity) {
        return false;
    }
    return true;
}

bool game_instance_config_validate(const UGameInstanceConfig *config, u8 *actor_capacity, UCollisionManagerConfig *collision_config) {
    if (config == NULL || actor_capacity == NULL || collision_config == NULL) {
        return false;
    }
    if ((config->level_graph == NULL) == (config->initial_level == NULL) || !unsigned_tlss_scale_config_is_valid(&config->tlss)) {
        return false;
    }
    if (config->player_capacity == 0u || config->player_capacity > U_INPUT_PLAYER_CAPACITY || config->refresh_rate == 0u) {
        return false;
    }

    const u16 total = unsigned_game_actor_capacity(config->player_capacity, config->npc_capacity, config->object_capacity, config->projectile_capacity);
    if (total == 0u || total > UINT8_MAX) {
        return false;
    }

    *actor_capacity = (u8)total;
    if (!game_instance_actor_storage_is_valid(config, *actor_capacity)) {
        return false;
    }

    *collision_config = game_collision_config_build(config->player_capacity, config->npc_capacity, config->object_capacity, config->projectile_capacity);
    return game_instance_collision_storage_is_valid(config, collision_config);
}
