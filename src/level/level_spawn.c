/**
 * @file level_spawn.c
 * @brief Implements level-definition spawning into fixed actor pools.
 */

#include "actor/npc_pool.h"
#include "actor/object_pool.h"
#include "level/level_internal.h"

/** Reserves NPC/character runtime slots and initializes NPCs declared by the level definition. */
static bool level_spawn_npcs(ULevel *level, const ULevelDefinition *definition, void *context) {
    UPoolInstanceContainer *pool = unsigned_level_npc_pool(level);
    const ULevelSpawnStorage *storage = &level->spawn_storage;

    if (definition->npc_spawn_count == 0u) {
        return true;
    }

    if (pool == NULL || definition->npc_spawns == NULL || definition->npc_spawn_count > pool->capacity || definition->npc_spawn_count > storage->npc_capacity || storage->npcs == NULL || storage->npc_characters == NULL) {
        return false;
    }

    for (u8 i = 0u; i < definition->npc_spawn_count; ++i) {
        const ULevelNpcSpawnDefinition *spawn = &definition->npc_spawns[i];
        UNpc *npc = &storage->npcs[i];
        UCharacter *character = &storage->npc_characters[i];

        if (spawn->init == NULL) {
            return false;
        }

        *npc = (UNpc){ 0 };
        *character = (UCharacter){ 0 };
        npc->character = character;
        npc->activity = spawn->dormant ? U_NPC_ACTIVITY_DORMANT : U_NPC_ACTIVITY_ACTIVE;

        if (!spawn->init(level, npc, character, spawn, context) || npc->character != character) {
            return false;
        }

        character->actor.position = spawn->position;
        if (unsigned_npc_pool_reserve(pool, npc) == NULL) {
            return false;
        }
    }

    return true;
}

/** Reserves object runtime slots and initializes objects declared by the level definition. */
static bool level_spawn_objects(ULevel *level, const ULevelDefinition *definition, void *context) {
    UPoolInstanceContainer *pool = unsigned_level_object_pool(level);
    const ULevelSpawnStorage *storage = &level->spawn_storage;

    if (definition->object_spawn_count == 0u) {
        return true;
    }

    if (pool == NULL || definition->object_spawns == NULL || definition->object_spawn_count > pool->capacity || definition->object_spawn_count > storage->object_capacity || storage->objects == NULL) {
        return false;
    }

    for (u8 i = 0u; i < definition->object_spawn_count; ++i) {
        const ULevelObjectSpawnDefinition *spawn = &definition->object_spawns[i];
        UObject *object = &storage->objects[i];

        if (spawn->init == NULL) {
            return false;
        }

        *object = (UObject){ 0 };
        object->static_collision = spawn->static_collision;
        if (!spawn->init(level, object, spawn, context)) {
            return false;
        }

        object->actor.position = spawn->position;
        if (unsigned_object_pool_reserve(pool, object) == NULL) {
            return false;
        }
    }

    return true;
}

bool level_spawn_load(ULevel *level, const ULevelDefinition *definition, void *context) {
    return level_spawn_npcs(level, definition, context) && level_spawn_objects(level, definition, context) && level_actor_sync_pools(level);
}
