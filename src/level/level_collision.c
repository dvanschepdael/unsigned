/**
 * @file level_collision.c
 * @brief Orchestrates collision registration and hit/projectile resolution for a level.
 */

#include "level/level_collision.h"

#include "actor/npc.h"
#include "actor/object.h"
#include "actor/player.h"
#include "collision/actor_collision.h"
#include "collision/hit_detection.h"
#include "collision/projectile_collision.h"
#include "level/level_internal.h"

/** Register every active player actor in the level's dynamic collision layers. */
static bool level_collision_register_players(UPoolInstanceContainer *pool, UCollisionManager *manager) {
    bool complete = true;
    if (pool == NULL || manager == NULL) {
        return false;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (instance->active && instance->args != NULL && !unsigned_player_collision_register(instance->args, manager)) {
            complete = false;
        }
    }
    return complete;
}

/** Register every active NPC actor in the level's dynamic collision layers. */
static bool level_collision_register_npcs(UPoolInstanceContainer *pool, UCollisionManager *manager) {
    bool complete = true;
    if (pool == NULL || manager == NULL) {
        return false;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (instance->active && instance->args != NULL && !unsigned_npc_collision_register(instance->args, manager)) {
            complete = false;
        }
    }
    return complete;
}

/** Register either static or dynamic objects, according to the object's static_collision flag. */
static bool level_collision_register_objects(UPoolInstanceContainer *pool, UCollisionManager *manager, bool persistent) {
    bool complete = true;
    if (pool == NULL || manager == NULL) {
        return false;
    }

    for (u8 i = 0u; i < pool->capacity; ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (!instance->active || instance->args == NULL) {
            continue;
        }

        UObject *object = instance->args;
        if (object->static_collision != persistent) {
            continue;
        }

        const bool registered = persistent ? unsigned_actor_collision_register_static(&object->actor, manager) : unsigned_actor_collision_register(&object->actor, manager);
        if (!registered) {
            complete = false;
        }
    }
    return complete;
}

/** Register all frame-rebuilt actor collision. Failures are accumulated so every pool still gets a chance to register. */
static bool level_collision_register_dynamic(ULevel *level) {
    bool complete = true;
    UCollisionManager *manager = &level->collision.manager;

    if (!level_collision_register_players(unsigned_level_player_pool(level), manager)) {
        complete = false;
    }
    if (!level_collision_register_npcs(unsigned_level_npc_pool(level), manager)) {
        complete = false;
    }
    if (!level_collision_register_objects(unsigned_level_object_pool(level), manager, false)) {
        complete = false;
    }
    return complete;
}

/**
 * Build persistent object collision after a level is loaded.
 * Static boxes remain in UCollisionManager while per-frame dynamic layers are cleared/rebuilt.
 */
bool level_collision_build_static(ULevel *level) {
    if (level == NULL) {
        return false;
    }

    level->collision.static_registration_complete = level_collision_register_objects(unsigned_level_object_pool(level), &level->collision.manager, true);
    return level->collision.static_registration_complete;
}

/**
 * Complete the level collision pipeline for the current frame.
 * Any incomplete registration invalidates collision for the frame. Continuing with only a
 * subset of bodies/hitboxes would make results depend on registration order.
 */
void level_collision_detect(ULevel *level) {
    if (level == NULL || level->definition == NULL) {
        return;
    }

    ULevelCollisionRuntime *collision = &level->collision;
    UPoolInstanceContainer *projectiles = unsigned_level_projectile_pool(level);

    collision->registration_complete = collision->static_registration_complete && level_collision_register_dynamic(level);
    if (!collision->registration_complete) {
        collision->hits = (UCollisionHitContainer){ 0 };
        return;
    }

    if (!level_actor_sync_pools(level)) {
        collision->hits = (UCollisionHitContainer){ 0 };
        return;
    }

    unsigned_actor_container_sort(&level->actors, level->definition->actor_order);
    if (!unsigned_collision_actor_index_build(&collision->actor_index, &level->actors, &collision->manager)) {
        collision->hits = (UCollisionHitContainer){ 0 };
        return;
    }

    unsigned_collision_projectiles_resolve(&collision->manager, &collision->actor_index, projectiles, &level->tlss);
    if (!unsigned_collision_hit_detect(&collision->hits, &collision->manager, &collision->actor_index, &level->tlss)) {
        collision->hits = (UCollisionHitContainer){ 0 };
    }
}

bool unsigned_level_collision_registration_complete(const ULevel *level) {
    return level != NULL && level->collision.registration_complete;
}

bool unsigned_level_collision_registration_overflowed(const ULevel *level) {
    return level != NULL && unsigned_physics_collision_has_registration_overflow(&level->collision.manager);
}

/** Return true when attack hit results were truncated because the fixed hit buffer filled. */
bool unsigned_level_collision_hits_overflowed(const ULevel *level) {
    return level != NULL && level->collision.hits.overflowed;
}
