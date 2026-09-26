/**
 * @file level_actor.c
 * @brief Implements level-wide actor pool synchronization and ticking.
 */

#include "actor/npc_pool.h"
#include "actor/object_pool.h"
#include "actor/player_pool.h"
#include "actor/projectile_pool.h"
#include "level/level_internal.h"

void level_actor_clear(ULevel *level) {
    UPoolInstanceContainer *players = &level->actor_pools->players;
    UPoolInstanceContainer *npcs = &level->actor_pools->npcs;
    UPoolInstanceContainer *objects = &level->actor_pools->objects;
    UPoolInstanceContainer *projectiles = &level->actor_pools->projectiles;

    for (u8 i = 0u; i < unsigned_pool_iteration_end(players); ++i) {
        UPoolInstance *instance = &players->instances[i];
        if (instance->active) {
            unsigned_player_pool_release(players, &level->gameplay->abilities, instance);
        }
    }

    for (u8 i = 0u; i < unsigned_pool_iteration_end(npcs); ++i) {
        UPoolInstance *instance = &npcs->instances[i];
        if (instance->active) {
            unsigned_npc_pool_release(npcs, instance);
        }
    }

    for (u8 i = 0u; i < unsigned_pool_iteration_end(objects); ++i) {
        UPoolInstance *instance = &objects->instances[i];
        if (instance->active) {
            unsigned_object_pool_release(objects, instance);
        }
    }

    for (u8 i = 0u; i < unsigned_pool_iteration_end(projectiles); ++i) {
        UPoolInstance *instance = &projectiles->instances[i];
        if (instance->active) {
            unsigned_projectile_pool_release(projectiles, instance);
        }
    }

    level->actors.count = 0u;
    level->actors.layout_revision = 0u;
    level->actor_sync = (ULevelActorSyncRuntime){0};
}

void level_actor_sync_pools(ULevel *level) {
    const UPoolInstanceContainer *players = &level->actor_pools->players;
    const UPoolInstanceContainer *npcs = &level->actor_pools->npcs;
    const UPoolInstanceContainer *objects = &level->actor_pools->objects;
    const UPoolInstanceContainer *projectiles = &level->actor_pools->projectiles;

    /* Pool reserve/release owns membership revisions. Most frames do not change membership, so
     * avoid rescanning every actor merely to rediscover that the same pool slots are still alive. */
    if (level->actor_sync.player_revision == players->revision && level->actor_sync.npc_revision == npcs->revision && level->actor_sync.object_revision == objects->revision &&
        level->actor_sync.projectile_revision == projectiles->revision) {
        return;
    }

    /* Membership revisions changed: rebuild directly from the four authoritative pools.
     * This is linear in live pool spans and avoids duplicate searches or structural rechecks. */
    level->actors.count = 0u;
    ++level->actors.layout_revision;

    for (u8 i = 0u; i < unsigned_pool_iteration_end(players); ++i) {
        const UPoolInstance *instance = &players->instances[i];

        if (!instance->active) {
            continue;
        }
        UPlayer *player = instance->args;
        level->actors.instances[level->actors.count++] = &player->character->actor;
    }

    for (u8 i = 0u; i < unsigned_pool_iteration_end(npcs); ++i) {
        const UPoolInstance *instance = &npcs->instances[i];

        if (!instance->active) {
            continue;
        }
        UNpc *npc = instance->args;
        level->actors.instances[level->actors.count++] = &npc->character->actor;
    }

    for (u8 i = 0u; i < unsigned_pool_iteration_end(objects); ++i) {
        const UPoolInstance *instance = &objects->instances[i];

        if (!instance->active) {
            continue;
        }
        UObject *object = instance->args;
        level->actors.instances[level->actors.count++] = &object->actor;
    }

    for (u8 i = 0u; i < unsigned_pool_iteration_end(projectiles); ++i) {
        const UPoolInstance *instance = &projectiles->instances[i];

        if (!instance->active) {
            continue;
        }
        UProjectile *projectile = instance->args;
        level->actors.instances[level->actors.count++] = projectile->actor;
    }

    level->actor_sync.player_revision = players->revision;
    level->actor_sync.npc_revision = npcs->revision;
    level->actor_sync.object_revision = objects->revision;
    level->actor_sync.projectile_revision = projectiles->revision;
}

void level_actor_prepare_frame(ULevel *level) {
    level_actor_sync_pools(level);
    unsigned_actor_container_sort(&level->actors, level->definition->actor_order);
}

void level_actor_tick(ULevel *level, UInputManager *input) {
    UPoolInstanceContainer *players = &level->actor_pools->players;
    UPoolInstanceContainer *npcs = &level->actor_pools->npcs;
    UPoolInstanceContainer *objects = &level->actor_pools->objects;
    UPoolInstanceContainer *projectiles = &level->actor_pools->projectiles;

    unsigned_player_pool_tick(players, input, &level->gameplay->abilities);
    unsigned_npc_pool_tick(npcs, &level->tlss);
    unsigned_object_pool_tick(objects);
    unsigned_projectile_pool_tick(projectiles);
}
