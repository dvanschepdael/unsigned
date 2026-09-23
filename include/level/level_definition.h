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
#include "level/level_camera.h"

typedef struct ULevel ULevel;
typedef struct ULevelDefinition ULevelDefinition;
typedef struct ULevelNpcSpawnDefinition ULevelNpcSpawnDefinition;
typedef struct ULevelObjectSpawnDefinition ULevelObjectSpawnDefinition;

struct UBackgroundLayerDefinition;
struct UNpc;
struct UCharacter;
struct UObject;

typedef void (*ULevelLoad)(ULevel *level, const ULevelDefinition *definition, void *context);
typedef void (*ULevelUnload)(ULevel *level, const ULevelDefinition *definition, void *context);
typedef void (*ULevelCallback)(ULevel *level, const ULevelDefinition *definition, void *context);
typedef void (*ULevelNpcSpawnInit)(ULevel *level, struct UNpc *npc, struct UCharacter *character, const ULevelNpcSpawnDefinition *spawn, void *context);
typedef void (*ULevelObjectSpawnInit)(ULevel *level, struct UObject *object, const ULevelObjectSpawnDefinition *spawn, void *context);

struct ULevelNpcSpawnDefinition {
    /** Content-specific immutable data forwarded to `init`. */
    const void *data;
    /** Wires the reserved NPC/character runtime. Authored content must satisfy its dependencies. */
    ULevelNpcSpawnInit init;
    /** Initial simulation class; dormant NPCs can be scheduled at a lower TLSS cadence. */
    bool dormant;
    Vec2 position;
};

struct ULevelObjectSpawnDefinition {
    const void *data;
    /** Initializes the reserved object runtime from authored content. */
    ULevelObjectSpawnInit init;
    /** Static collision is registered once at load instead of rebuilt every frame. */
    bool static_collision;
    Vec2 position;
};

/**
 * Immutable authored content for one loadable level.
 *
 * @invariant Every declared NPC/object spawn has a non-NULL `init` callback.
 * @invariant Declared spawn counts fit both their configured actor pools and `ULevelSpawnStorage`.
 * @invariant A spawned NPC keeps the `UCharacter` supplied to its `init` callback.
 * @invariant `background_layer_count <= UNSIGNED_BACKGROUND_MAX_LAYERS` and referenced arrays
 * remain alive for the complete loaded-level lifetime.
 */
struct ULevelDefinition {
    /** Optional content-specific data shared by level callbacks. */
    const void *data;
    /** Optional Beat'Em Up camera policy; NULL keeps the level camera fixed at the origin. */
    const ULevelCameraDefinition *camera;
    /** Optional setup hook executed before declared spawns. Authored dependencies/capacities are preconditions. */
    ULevelLoad load;
    /** Optional cleanup counterpart for content created by `load`. */
    ULevelUnload unload;
    /** Called after backgrounds, spawns and static collision are ready. */
    ULevelCallback enter;
    /** Called before unload while the level content is still available. */
    ULevelCallback exit;
    /** Ordering policy used for the level-wide actor view before collision/render passes. */
    UActorSort actor_order;
    /**
     * Keep hardware columns reserved for active actors even while culled.
     * This trades sprite-index space for much lower relocation/SCB1 churn as actors
     * enter and leave the viewport.
     */
    bool stable_actor_sprite_ranges;
    /** Skip dynamic NPC collision while the level AI classifies an NPC as off-screen/dormant. */
    bool cull_offscreen_npc_collision;
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
