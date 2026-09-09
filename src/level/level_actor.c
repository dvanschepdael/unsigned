/**
 * @file level_actor.c
 * @brief Implements level-wide actor pool synchronization and ticking.
 */

#include "actor/npc_pool.h"
#include "actor/object_pool.h"
#include "actor/player_pool.h"
#include "actor/projectile_pool.h"
#include "level/level_internal.h"

UPoolInstanceContainer *unsigned_level_player_pool(ULevel *level) {
    return level != NULL && level->actor_pools != NULL ? &level->actor_pools->players : NULL;
}

UPoolInstanceContainer *unsigned_level_npc_pool(ULevel *level) {
    return level != NULL && level->actor_pools != NULL ? &level->actor_pools->npcs : NULL;
}

UPoolInstanceContainer *unsigned_level_object_pool(ULevel *level) {
    return level != NULL && level->actor_pools != NULL ? &level->actor_pools->objects : NULL;
}

UPoolInstanceContainer *unsigned_level_projectile_pool(ULevel *level) {
    return level != NULL && level->actor_pools != NULL ? &level->actor_pools->projectiles : NULL;
}

void level_actor_release_pools(ULevel *level) {
    UPoolInstanceContainer *players = unsigned_level_player_pool(level);
    UPoolInstanceContainer *npcs = unsigned_level_npc_pool(level);
    UPoolInstanceContainer *objects = unsigned_level_object_pool(level);
    UPoolInstanceContainer *projectiles = unsigned_level_projectile_pool(level);

    if (players == NULL || npcs == NULL || objects == NULL || projectiles == NULL) {
        return;
    }

    for (u8 i = 0u; i < players->capacity; ++i) {
        UPoolInstance *instance = &players->instances[i];
        if (instance->active) {
            unsigned_player_pool_release(players, level->gameplay != NULL ? &level->gameplay->abilities : NULL, instance);
        }
    }

    for (u8 i = 0u; i < npcs->capacity; ++i) {
        UPoolInstance *instance = &npcs->instances[i];
        if (instance->active) {
            unsigned_npc_pool_release(npcs, instance);
        }
    }

    for (u8 i = 0u; i < objects->capacity; ++i) {
        UPoolInstance *instance = &objects->instances[i];
        if (instance->active) {
            unsigned_object_pool_release(objects, instance);
        }
    }

    for (u8 i = 0u; i < projectiles->capacity; ++i) {
        UPoolInstance *instance = &projectiles->instances[i];
        if (instance->active) {
            unsigned_projectile_pool_release(projectiles, instance);
        }
    }
}

/** Appends one active actor pointer to the level-wide actor view when capacity permits. */
static bool level_actor_append(ULevel *level, UActor *actor) {
    if (level == NULL || actor == NULL || !actor->active || level->actors.instances == NULL) {
        return false;
    }

    for (u8 i = 0u; i < level->actors.count; ++i) {
        if (level->actors.instances[i] == actor) {
            return true;
        }
    }

    if (level->actors.count >= level->actors.capacity) {
        return false;
    }

    level->actors.instances[level->actors.count++] = actor;
    return true;
}

bool level_actor_sync_pools(ULevel *level) {
    const UPoolInstanceContainer *players = unsigned_level_player_pool(level);
    const UPoolInstanceContainer *npcs = unsigned_level_npc_pool(level);
    const UPoolInstanceContainer *objects = unsigned_level_object_pool(level);
    const UPoolInstanceContainer *projectiles = unsigned_level_projectile_pool(level);

    if (players == NULL || npcs == NULL || objects == NULL || projectiles == NULL) {
        return false;
    }

    {
        u8 write = 0u;
        for (u8 read = 0u; read < level->actors.count; ++read) {
            UActor *actor = level->actors.instances[read];
            if (actor != NULL && actor->active) {
                level->actors.instances[write++] = actor;
            }
        }
        level->actors.count = write;
    }

    {
        const u16 expected_count = (u16)((u16)players->count + (u16)npcs->count + (u16)objects->count + (u16)projectiles->count);
        if (expected_count > level->actors.capacity) {
            return false;
        }
        if ((u16)level->actors.count == expected_count) {
            return true;
        }
    }

    for (u8 i = 0u; i < players->capacity; ++i) {
        const UPoolInstance *instance = &players->instances[i];

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        UPlayer *player = instance->args;
        if (player->character == NULL || !level_actor_append(level, &player->character->actor)) {
            return false;
        }
    }

    for (u8 i = 0u; i < npcs->capacity; ++i) {
        const UPoolInstance *instance = &npcs->instances[i];

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        UNpc *npc = instance->args;
        if (npc->character == NULL || !level_actor_append(level, &npc->character->actor)) {
            return false;
        }
    }

    for (u8 i = 0u; i < objects->capacity; ++i) {
        const UPoolInstance *instance = &objects->instances[i];

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        UObject *object = instance->args;
        if (!level_actor_append(level, &object->actor)) {
            return false;
        }
    }

    for (u8 i = 0u; i < projectiles->capacity; ++i) {
        const UPoolInstance *instance = &projectiles->instances[i];

        if (!instance->active || instance->args == NULL) {
            continue;
        }

        UProjectile *projectile = instance->args;
        if (!level_actor_append(level, projectile->actor)) {
            return false;
        }
    }

    return true;
}

void level_actor_tick(ULevel *level, UInputManager *input) {
    UPoolInstanceContainer *players = unsigned_level_player_pool(level);
    UPoolInstanceContainer *npcs = unsigned_level_npc_pool(level);
    UPoolInstanceContainer *objects = unsigned_level_object_pool(level);
    UPoolInstanceContainer *projectiles = unsigned_level_projectile_pool(level);

    unsigned_player_pool_tick(players, input, &level->gameplay->abilities);
    unsigned_npc_pool_tick(npcs);
    unsigned_object_pool_tick(objects);
    unsigned_projectile_pool_tick(projectiles);
}
