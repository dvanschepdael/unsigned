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

typedef struct ULevelDynamicCollisionState {
    bool has_hitboxes;
    bool has_boxes;
} ULevelDynamicCollisionState;

/** Probe one actor for an attack source without transforming its hurtbox/hitbox. */
static void level_collision_probe_actor(UActor *actor, ULevelDynamicCollisionState *state) {
    state->has_hitboxes = state->has_hitboxes || unsigned_actor_collision_probe(actor);
}

/** Lightweight attack-source scan used by the idle-frame fast path. */
static ULevelDynamicCollisionState level_collision_probe_dynamic(ULevel *level) {
    ULevelDynamicCollisionState state = {0};
    const bool active_npcs_only = level->definition->cull_offscreen_npc_collision;
    UPoolInstanceContainer *players = &level->actor_pools->players;
    UPoolInstanceContainer *npcs = &level->actor_pools->npcs;
    UPoolInstanceContainer *objects = &level->actor_pools->objects;

    u8 remaining = players->count;
    for (u8 i = 0u; i < unsigned_pool_iteration_end(players) && remaining > 0u; ++i) {
        UPoolInstance *instance = &players->instances[i];
        if (!instance->active) {
            continue;
        }
        --remaining;
        UPlayer *player = instance->args;
        level_collision_probe_actor(&player->character->actor, &state);
    }

    remaining = npcs->count;
    for (u8 i = 0u; i < unsigned_pool_iteration_end(npcs) && remaining > 0u; ++i) {
        UPoolInstance *instance = &npcs->instances[i];
        if (!instance->active) {
            continue;
        }
        --remaining;
        UNpc *npc = instance->args;
        if (active_npcs_only && npc->activity != U_NPC_ACTIVITY_ACTIVE) {
            unsigned_actor_collision_deactivate_frame(&npc->character->actor);
            continue;
        }
        level_collision_probe_actor(&npc->character->actor, &state);
    }

    remaining = objects->count;
    for (u8 i = 0u; i < unsigned_pool_iteration_end(objects) && remaining > 0u; ++i) {
        UPoolInstance *instance = &objects->instances[i];
        if (!instance->active) {
            continue;
        }
        --remaining;
        UObject *object = instance->args;
        if (!object->static_collision) {
            level_collision_probe_actor(&object->actor, &state);
        }
    }
    return state;
}

/**
 * Refresh one actor and publish its current boxes in the same traversal.
 *
 * Keeping materialization at the level boundary avoids a second pool walk after refresh while
 * reusing the actor collision API as the owner of frame transforms and manager registration.
 */
static void level_collision_materialize_actor(UActor *actor, UCollisionManager *manager, ULevelDynamicCollisionState *state) {
    unsigned_actor_collision_register(actor, manager, false);

    const bool has_hitbox = actor->collision.hitbox_channel != U_COLLISION_CHANNEL_NONE;
    state->has_hitboxes = state->has_hitboxes || has_hitbox;
    state->has_boxes = state->has_boxes || has_hitbox || actor->collision.hurtbox_channel != U_COLLISION_CHANNEL_NONE;
}

/** Materialize active player collision only on frames that actually execute collision queries. */
static void level_collision_materialize_players(UPoolInstanceContainer *pool, UCollisionManager *manager, ULevelDynamicCollisionState *state) {
    u8 remaining = pool->count;
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool) && remaining > 0u; ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (!instance->active) {
            continue;
        }
        --remaining;
        UPlayer *player = instance->args;
        level_collision_materialize_actor(&player->character->actor, manager, state);
    }
}

/** Materialize active NPC collision, retiring frame state for NPCs excluded by level policy. */
static void level_collision_materialize_npcs(UPoolInstanceContainer *pool, UCollisionManager *manager, bool active_only, ULevelDynamicCollisionState *state) {
    u8 remaining = pool->count;
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool) && remaining > 0u; ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (!instance->active) {
            continue;
        }
        --remaining;
        UNpc *npc = instance->args;
        if (active_only && npc->activity != U_NPC_ACTIVITY_ACTIVE) {
            unsigned_actor_collision_deactivate_frame(&npc->character->actor);
            continue;
        }

        level_collision_materialize_actor(&npc->character->actor, manager, state);
    }
}

/** Materialize non-persistent object collision; static objects remain registered from level load. */
static void level_collision_materialize_objects(UPoolInstanceContainer *pool, UCollisionManager *manager, ULevelDynamicCollisionState *state) {
    u8 remaining = pool->count;
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool) && remaining > 0u; ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (!instance->active) {
            continue;
        }
        --remaining;
        UObject *object = instance->args;
        if (object->static_collision) {
            continue;
        }
        level_collision_materialize_actor(&object->actor, manager, state);
    }
}

/**
 * Refresh and register all frame-owned boxes in one pass per pool.
 * The collision manager must have had its dynamic tail cleared before this function is called.
 */
static ULevelDynamicCollisionState level_collision_materialize_dynamic(ULevel *level) {
    ULevelDynamicCollisionState state = {0};
    UCollisionManager *manager = &level->collision.manager;
    const bool active_npcs_only = level->definition->cull_offscreen_npc_collision;

    level_collision_materialize_players(&level->actor_pools->players, manager, &state);
    level_collision_materialize_npcs(&level->actor_pools->npcs, manager, active_npcs_only, &state);
    level_collision_materialize_objects(&level->actor_pools->objects, manager, &state);
    return state;
}

/** Register persistent object collision once during level load. */
static void level_collision_register_static_objects(UPoolInstanceContainer *pool, UCollisionManager *manager, bool *has_hitboxes) {
    *has_hitboxes = false;

    u8 remaining = pool->count;
    for (u8 i = 0u; i < unsigned_pool_iteration_end(pool) && remaining > 0u; ++i) {
        UPoolInstance *instance = &pool->instances[i];
        if (!instance->active) {
            continue;
        }
        --remaining;
        UObject *object = instance->args;
        if (!object->static_collision) {
            continue;
        }

        unsigned_actor_collision_register(&object->actor, manager, true);
        if (object->actor.collision.hitbox_channel != U_COLLISION_CHANNEL_NONE) {
            *has_hitboxes = true;
        }
    }
}

/**
 * Build persistent object collision after a level is loaded.
 * Static boxes remain in UCollisionManager while per-frame dynamic layers are cleared/rebuilt.
 */
void level_collision_build_static(ULevel *level) {
    level_collision_register_static_objects(&level->actor_pools->objects, &level->collision.manager, &level->collision.static_has_hitboxes);
}

/**
 * Complete the level collision pipeline for the current frame.
 * Static and dynamic collision storage capacities are composition contracts, so this hot path
 * focuses only on avoiding work when no gameplay source can produce a query.
 */
void level_collision_detect(ULevel *level) {
    ULevelCollisionRuntime *collision = &level->collision;
    UPoolInstanceContainer *projectiles = &level->actor_pools->projectiles;
    ULevelDynamicCollisionState dynamic = {0};

    const bool has_projectiles = projectiles->count > 0u;
    const bool has_persistent_hitboxes = collision->static_has_hitboxes;

    /* Projectile frames already require target hurtboxes, so probing the same pools first is pure
     * overhead. On non-projectile frames, keep the lightweight probe to preserve the idle fast
     * path without transforming or registering every hurtbox. */
    if (!has_projectiles) {
        dynamic = level_collision_probe_dynamic(level);

        if (!dynamic.has_hitboxes && !has_persistent_hitboxes) {
            /* No gameplay source can query dynamic boxes this frame. Probe only maintains the
             * attack-edge cache; a previous query frame is cleared once when returning to idle. */
            if (collision->dynamic_registration_populated) {
                unsigned_physics_collision_clear_dynamic(&collision->manager);
                collision->dynamic_registration_populated = false;
            }
            collision->actor_index = (UCollisionActorIndex){0};
            collision->hits = (UCollisionHitContainer){0};
            return;
        }
    }

    /* Query-producing frames clear the old dynamic tail once, then refresh and publish each
     * actor in the same pool traversal. This avoids a dedicated registration pass. */
    unsigned_physics_collision_clear_dynamic(&collision->manager);
    collision->dynamic_registration_populated = false;
    dynamic = level_collision_materialize_dynamic(level);
    collision->dynamic_registration_populated = dynamic.has_boxes;

    unsigned_collision_actor_index_build(&collision->actor_index, &level->actors, &collision->manager);

    if (has_projectiles) {
        unsigned_collision_projectiles_resolve(&collision->manager, &collision->actor_index, projectiles, &level->tlss);
    }

    const bool has_hitboxes = dynamic.has_hitboxes || has_persistent_hitboxes;
    if (has_hitboxes) {
        unsigned_collision_hit_detect(&collision->hits, &collision->manager, &collision->actor_index, &level->tlss);
    } else {
        collision->hits = (UCollisionHitContainer){0};
    }
}
