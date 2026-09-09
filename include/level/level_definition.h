/**
 * @file level_definition.h
 * @brief Static level content and callback definition.
 *
 * Definitions are content data: the engine retains their pointers while the level is loaded,
 * so the definition and referenced arrays must outlive the runtime level. Mutable state belongs
 * in `ULevel`, spawned actors, or the caller-provided `context`.
 */

#ifndef UNSIGNED_LEVEL_DEFINITION_H
#define UNSIGNED_LEVEL_DEFINITION_H

#include "actor/actor_sort.h"
#include "core/types.h"

typedef struct ULevel ULevel;
typedef struct ULevelDefinition ULevelDefinition;
typedef struct ULevelNpcSpawnDefinition ULevelNpcSpawnDefinition;
typedef struct ULevelObjectSpawnDefinition ULevelObjectSpawnDefinition;

struct UBackgroundLayerDefinition;
struct UNpc;
struct UCharacter;
struct UObject;

typedef bool (*ULevelLoad)(ULevel *level, const ULevelDefinition *definition, void *context);
typedef void (*ULevelUnload)(ULevel *level, const ULevelDefinition *definition, void *context);
typedef void (*ULevelCallback)(ULevel *level, const ULevelDefinition *definition, void *context);
typedef bool (*ULevelNpcSpawnInit)(ULevel *level, struct UNpc *npc, struct UCharacter *character, const ULevelNpcSpawnDefinition *spawn, void *context);
typedef bool (*ULevelObjectSpawnInit)(ULevel *level, struct UObject *object, const ULevelObjectSpawnDefinition *spawn, void *context);

struct ULevelNpcSpawnDefinition {
    /** Content-specific immutable data forwarded to `init`. */
    const void *data;
    /** Must finish wiring the reserved NPC/character runtime; false aborts the level load. */
    ULevelNpcSpawnInit init;
    /** Initial simulation class; dormant NPCs can be scheduled at a lower TLSS cadence. */
    bool dormant;
    Vec2 position;
};

struct ULevelObjectSpawnDefinition {
    const void *data;
    /** Initializes the reserved object; false aborts the level load. */
    ULevelObjectSpawnInit init;
    /** Static collision is registered once at load instead of rebuilt every frame. */
    bool static_collision;
    Vec2 position;
};

struct ULevelDefinition {
    /** Optional content-specific data shared by level callbacks. */
    const void *data;
    /** Optional setup hook executed before declared spawns; false triggers load rollback. */
    ULevelLoad load;
    /** Cleanup counterpart for custom load work; also called when a started load later fails. */
    ULevelUnload unload;
    /** Called after backgrounds, spawns and static collision are ready. */
    ULevelCallback enter;
    /** Called before unload while the level content is still available. */
    ULevelCallback exit;
    /** Ordering policy used for the level-wide actor view before collision/render passes. */
    UActorSort actor_order;
    /** Game-specific interpretation of frame-local attacker/target hits. */
    ULevelCallback resolve_hits;
    u16 backdrop_color;
    u8 background_layer_count;
    u8 npc_spawn_count;
    u8 object_spawn_count;
    const struct UBackgroundLayerDefinition *background_layers;
    const ULevelNpcSpawnDefinition *npc_spawns;
    const ULevelObjectSpawnDefinition *object_spawns;
};

#endif
