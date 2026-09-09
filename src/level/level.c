/**
 * @file level.c
 * @brief Coordinates level lifetime and the top-level order of per-frame level subsystems.
 */

#include "level/level.h"

#include "gameplay/ability_pool.h"
#include "gameplay/cue_pool.h"
#include "gameplay/effect_pool.h"
#include "level/level_internal.h"

/** Clear all mutable state owned by the currently loaded level content. */
static void level_clear_loaded_content(ULevel *level) {
    if (level == NULL) {
        return;
    }

    unsigned_gameplay_runtime_clear(level->gameplay);
    level_actor_release_pools(level);
    level->actors.count = 0u;
    unsigned_physics_collision_clear(&level->collision.manager);
    level->collision.static_registration_complete = false;
    level->collision.registration_complete = false;
    level->definition = NULL;
    level->context = NULL;
}

/** Roll back every subsystem touched by a failed load and return false for call-site chaining. */
static bool level_load_failed(ULevel *level, bool custom_load_started) {
    const ULevelDefinition *definition = level->definition;
    void *context = level->context;

    if (custom_load_started && definition != NULL && definition->unload != NULL) {
        definition->unload(level, definition, context);
    }
    level_clear_loaded_content(level);
    return false;
}

/** Install background definitions, run the optional custom load hook, then spawn declared content. */
static bool level_load_content(ULevel *level, const ULevelDefinition *definition, void *context, bool *custom_load_started) {
    if (definition->background_layer_count > UNSIGNED_BACKGROUND_MAX_LAYERS || (definition->background_layer_count > 0u && definition->background_layers == NULL)) {
        return false;
    }

    for (u8 i = 0u; i < definition->background_layer_count; ++i) {
        if (!unsigned_background_set_layer(&level->background, i, &definition->background_layers[i])) {
            return false;
        }
    }

    if (definition->load != NULL) {
        *custom_load_started = true;
        if (!definition->load(level, definition, context)) {
            return false;
        }
    }

    return level_spawn_load(level, definition, context);
}

bool unsigned_level_init(ULevel *level, UActorPoolSet *actor_pools, UGameplayRuntime *gameplay, const ULevelSpawnStorage *spawn_storage, const UCollisionManagerConfig *collision_config, const UCollisionManagerStorage *collision_storage, UActor **actor_instances, u8 actor_capacity) {
    if (level == NULL || actor_pools == NULL || gameplay == NULL || spawn_storage == NULL || collision_config == NULL || collision_storage == NULL || actor_instances == NULL || actor_capacity == 0u ||
        (spawn_storage->npc_capacity > 0u && (spawn_storage->npcs == NULL || spawn_storage->npc_characters == NULL)) || (spawn_storage->object_capacity > 0u && spawn_storage->objects == NULL)) {
        return false;
    }

    *level = (ULevel){
        .actor_pools = actor_pools,
        .gameplay = gameplay,
        .spawn_storage = *spawn_storage,
        .tlss_config = {
            .ai = U_TLSS_SCALE_1,
            .collision = U_TLSS_SCALE_1,
        },
        .collision = {
            .config = *collision_config,
            .storage = *collision_storage,
            .static_registration_complete = true,
            .registration_complete = true,
        },
    };
    unsigned_actor_container_init(&level->actors, actor_instances, actor_capacity);
    return unsigned_physics_collision_init(&level->collision.manager, &level->collision.config, &level->collision.storage);
}

bool unsigned_level_set_tlss_config(ULevel *level, const UTLSSScaleConfig *tlss_config) {
    if (level == NULL || !unsigned_tlss_scale_config_is_valid(tlss_config)) {
        return false;
    }

    level->tlss_config = *tlss_config;
    level->tlss.scales = *tlss_config;
    return true;
}

bool unsigned_level_load(ULevel *level, const ULevelDefinition *definition, void *context) {
    bool custom_load_started = false;

    if (level == NULL || definition == NULL || definition->actor_order >= U_ACTOR_ORDER_COUNT || level->actor_pools == NULL || level->actors.instances == NULL || level->actors.capacity == 0u) {
        return false;
    }

    /* load() is a replace operation: release the previous scene before reusing its runtime. */
    unsigned_level_unload(level);

    UActorPoolSet *actor_pools = level->actor_pools;
    UGameplayRuntime *gameplay = level->gameplay;
    ULevelSpawnStorage spawn_storage = level->spawn_storage;
    UTLSSScaleConfig tlss_config = level->tlss_config;
    UCollisionManagerConfig collision_config = level->collision.config;
    UCollisionManagerStorage collision_storage = level->collision.storage;
    UActor **actor_instances = level->actors.instances;
    u8 actor_capacity = level->actors.capacity;

    *level = (ULevel){
        .actor_pools = actor_pools,
        .gameplay = gameplay,
        .spawn_storage = spawn_storage,
        .tlss_config = tlss_config,
        .collision = {
            .config = collision_config,
            .storage = collision_storage,
            .static_registration_complete = true,
            .registration_complete = true,
        },
        .definition = definition,
        .context = context,
    };
    unsigned_actor_container_init(&level->actors, actor_instances, actor_capacity);

    unsigned_tlss_init(&level->tlss);
    level->tlss.scales = tlss_config;
    if (!unsigned_physics_collision_init(&level->collision.manager, &level->collision.config, &level->collision.storage)) {
        return level_load_failed(level, custom_load_started);
    }
    unsigned_background_init(&level->background);

    if (!level_load_content(level, definition, context, &custom_load_started)) {
        return level_load_failed(level, custom_load_started);
    }

    if (!level_collision_build_static(level)) {
        return level_load_failed(level, custom_load_started);
    }

    if (definition->enter != NULL) {
        definition->enter(level, definition, context);
    }

    return true;
}

/** Advances the level by one scheduled engine frame. */
static void level_tick(ULevel *level, UInputManager *input, const UViewport *viewport) {
    if (level == NULL || level->definition == NULL || level->actor_pools == NULL || level->gameplay == NULL) {
        return;
    }

    unsigned_tlss_begin_frame(&level->tlss);
    unsigned_physics_collision_clear_dynamic(&level->collision.manager);

    level_actor_tick(level, input);
    level_ai_tick(level, viewport);
    unsigned_gameplay_ability_pool_tick(&level->gameplay->abilities);
    level_collision_detect(level);

    if (level->collision.hits.count > 0u && level->definition->resolve_hits != NULL) {
        level->definition->resolve_hits(level, level->definition, level->context);
    }

    unsigned_gameplay_effect_pool_tick(&level->gameplay->effects);
    unsigned_gameplay_cue_pool_tick(&level->gameplay->cues);
    unsigned_background_tick(&level->background);
}

void unsigned_level_tick(ULevel *level, UInputManager *input) {
    level_tick(level, input, NULL);
}

void unsigned_level_tick_with_viewport(ULevel *level, UInputManager *input, const UViewport *viewport) {
    level_tick(level, input, viewport);
}

void unsigned_level_unload(ULevel *level) {
    if (level == NULL || level->definition == NULL) {
        return;
    }

    if (level->definition->exit != NULL) {
        level->definition->exit(level, level->definition, level->context);
    }

    if (level->definition->unload != NULL) {
        level->definition->unload(level, level->definition, level->context);
    }

    level_clear_loaded_content(level);
}
