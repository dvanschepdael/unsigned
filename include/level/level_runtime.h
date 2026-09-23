/**
 * @file level_runtime.h
 * @brief Mutable runtime resources used by a loaded level.
 *
 * `ULevel` is intentionally separate from `ULevelDefinition`: definitions are immutable content,
 * while this file contains mutable state and borrowed fixed-capacity storage used during play.
 */

#ifndef UNSIGNED_LEVEL_RUNTIME_H
#define UNSIGNED_LEVEL_RUNTIME_H

#include "actor/actor.h"
#include "actor/npc_ai.h"
#include "actor/pools.h"
#include "collision/actor_index.h"
#include "collision/hit_detection.h"
#include "core/tlss/tlss.h"
#include "display/viewport/viewport.h"
#include "gameplay/runtime.h"
#include "level/background/background.h"
#include "level/level_definition.h"
#include "physics/collision.h"

struct UNpc;
struct UCharacter;
struct UObject;

/** Caller-owned runtime arrays used to materialize level-declared NPC/object spawns. */
typedef struct ULevelSpawnStorage {
    struct UNpc *npcs;
    struct UCharacter *npc_characters;
    struct UObject *objects;
} ULevelSpawnStorage;

typedef struct ULevelActorSyncRuntime {
    u16 player_revision;
    u16 npc_revision;
    u16 object_revision;
    u16 projectile_revision;
} ULevelActorSyncRuntime;

typedef struct ULevelCollisionRuntime {
    /** Derived index rebuilt after actor synchronization to prune gameplay hit searches. */
    UCollisionActorIndex actor_index;
    /** Frame-local attacker/target results consumed by the level `resolve_hits` callback. */
    UCollisionHitContainer hits;
    UCollisionManager manager;
    /** Cached while static collision is built; persistent geometry is immutable until level reload. */
    bool static_has_hitboxes;
    /** True only while frame-owned boxes are currently present in manager layer tails. */
    bool dynamic_registration_populated;
} ULevelCollisionRuntime;

struct ULevel {
    UTLSS tlss;
    UTLSSScaleConfig tlss_config;
    UActorContainer actors;
    ULevelActorSyncRuntime actor_sync;
    UActorPoolSet *actor_pools;
    ULevelSpawnStorage spawn_storage;
    UGameplayRuntime *gameplay;
    ULevelCollisionRuntime collision;
    UBackground background;
    UCamera camera;
    const ULevelDefinition *definition;
    void *context;
};

#endif
