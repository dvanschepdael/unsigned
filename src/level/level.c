/**
 * @file level.c
 * @brief Coordinates level lifetime and the top-level order of per-frame level subsystems.
 */

#include "level/level.h"

#include "gameplay/ability_pool.h"
#include "gameplay/cue_pool.h"
#include "gameplay/effect_pool.h"
#include "level/level_internal.h"
#include "level/level_camera.h"

/** Clear all mutable state owned by the currently loaded level content. */
static void level_clear_loaded_content(ULevel *level) {
    unsigned_gameplay_runtime_clear(level->gameplay);
    level_actor_release_pools(level);
    level->actors.count = 0u;
    level->actors.layout_revision = 0u;
    level->actor_sync = (ULevelActorSyncRuntime){0};
    unsigned_physics_collision_clear(&level->collision.manager);
    level->collision.actor_index = (UCollisionActorIndex){0};
    level->collision.hits = (UCollisionHitContainer){0};
    level->collision.static_has_hitboxes = false;
    level->collision.dynamic_registration_populated = false;
    level->definition = NULL;
    level->context = NULL;
}

/** Install background definitions, run the optional custom load hook, then spawn declared content. */
static void level_load_content(ULevel *level, const ULevelDefinition *definition, void *context) {
    for (u8 i = 0u; i < definition->background_layer_count; ++i) {
        unsigned_background_set_layer(&level->background, i, &definition->background_layers[i]);
    }

    if (definition->load != NULL) {
        definition->load(level, definition, context);
    }

    level_spawn_load(level, definition, context);
}

void unsigned_level_init(ULevel *level, const ULevelRuntimeConfig *config) {
    *level = (ULevel){
        .actor_pools = config->actor_pools,
        .gameplay = config->gameplay,
        .spawn_storage = *config->spawn_storage,
        .tlss_config =
            {
                .ai = U_TLSS_SCALE_1,
                .collision = U_TLSS_SCALE_1,
            },
    };
    unsigned_actor_container_init(&level->actors, config->actor_instances);
    unsigned_physics_collision_init(&level->collision.manager, config->collision_config, config->collision_layer_boxes);
}

void unsigned_level_set_tlss_config(ULevel *level, const UTLSSScaleConfig *tlss_config) {
    level->tlss_config = *tlss_config;
    level->tlss.scales = *tlss_config;
}

void unsigned_level_load(ULevel *level, const ULevelDefinition *definition, void *context) {

    /* load() is a replace operation: release the previous scene before reusing its runtime. */
    unsigned_level_unload(level);

    level->definition = definition;
    level->context = context;

    unsigned_tlss_init(&level->tlss);
    level->tlss.scales = level->tlss_config;
    unsigned_background_init(&level->background);
    unsigned_level_camera_init(level, definition->camera);

    level_load_content(level, definition, context);

    /* Backgrounds consume camera world position, including a non-zero level start before first render. */
    unsigned_background_tick(&level->background, level->camera.x);

    level_collision_build_static(level);

    if (definition->enter != NULL) {
        definition->enter(level, definition, context);
    }
}

/** Advances the loaded level by one scheduled engine frame. */
void unsigned_level_tick(ULevel *level, UInputManager *input, const UViewport *viewport) {
    unsigned_tlss_begin_frame(&level->tlss);

    level_actor_tick(level, input);
    level_ai_tick(level, viewport);
    unsigned_gameplay_ability_pool_tick(&level->gameplay->abilities);
    unsigned_level_camera_tick(level, viewport);

    /* Build/order the actor view once as shared frame data. Collision and rendering consume the
     * same stable ordering rather than maintaining their own copies. */
    level_actor_prepare_frame(level);
    level_collision_detect(level);

    if (level->collision.hits.count > 0u && level->definition->resolve_hits != NULL) {
        level->definition->resolve_hits(level, level->definition, level->context);
    }

    unsigned_gameplay_effect_pool_tick(&level->gameplay->effects);
    unsigned_gameplay_cue_pool_tick(&level->gameplay->cues);
    unsigned_background_tick(&level->background, level->camera.x);
}

void unsigned_level_unload(ULevel *level) {
    if (level->definition == NULL) {
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
