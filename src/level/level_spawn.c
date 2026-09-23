/**
 * @file level_spawn.c
 * @brief Materializes authored level spawns into caller-owned fixed actor pools.
 */

#include "actor/npc_pool.h"
#include "actor/object_pool.h"
#include "level/level_internal.h"

/** Materialize NPC definitions into their dedicated runtime storage and pool slots. */
static void level_spawn_npcs(ULevel *level, const ULevelDefinition *definition, void *context) {
    UPoolInstanceContainer *pool = &level->actor_pools->npcs;
    const ULevelSpawnStorage *storage = &level->spawn_storage;

    for (u8 i = 0u; i < definition->npc_spawn_count; ++i) {
        const ULevelNpcSpawnDefinition *spawn = &definition->npc_spawns[i];
        UNpc *npc = &storage->npcs[i];
        UCharacter *character = &storage->npc_characters[i];

        *npc = (UNpc){0};
        *character = (UCharacter){0};
        npc->character = character;
        npc->activity = spawn->dormant ? U_NPC_ACTIVITY_DORMANT : U_NPC_ACTIVITY_ACTIVE;

        spawn->init(level, npc, character, spawn, context);
        character->actor.position = spawn->position;
        (void)unsigned_npc_pool_reserve(pool, npc);
    }
}

/** Materialize object definitions into their dedicated runtime storage and pool slots. */
static void level_spawn_objects(ULevel *level, const ULevelDefinition *definition, void *context) {
    UPoolInstanceContainer *pool = &level->actor_pools->objects;
    const ULevelSpawnStorage *storage = &level->spawn_storage;

    for (u8 i = 0u; i < definition->object_spawn_count; ++i) {
        const ULevelObjectSpawnDefinition *spawn = &definition->object_spawns[i];
        UObject *object = &storage->objects[i];

        *object = (UObject){0};
        object->static_collision = spawn->static_collision;
        spawn->init(level, object, spawn, context);
        object->actor.position = spawn->position;
        (void)unsigned_object_pool_reserve(pool, object);
    }
}

void level_spawn_load(ULevel *level, const ULevelDefinition *definition, void *context) {
    level_spawn_npcs(level, definition, context);
    level_spawn_objects(level, definition, context);
    level_actor_sync_pools(level);
}
