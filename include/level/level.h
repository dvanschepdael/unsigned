/**
 * @file level.h
 * @brief Owns the runtime state and per-frame orchestration of one loaded game level.
 */

#ifndef UNSIGNED_LEVEL_H
#define UNSIGNED_LEVEL_H

#include "core/types.h"
#include "level/level_definition.h"

struct UActor;
struct UActorPoolSet;
struct UGameplayRuntime;
struct UInputManager;
struct UCollisionManagerConfig;
struct UCollisionBox;
struct ULevelSpawnStorage;
struct UTLSSScaleConfig;
struct UViewport;

typedef struct ULevelRuntimeConfig {
    struct UActorPoolSet *actor_pools;
    struct UGameplayRuntime *gameplay;
    const struct ULevelSpawnStorage *spawn_storage;
    const struct UCollisionManagerConfig *collision_config;
    const struct UCollisionBox **collision_layer_boxes;
    struct UActor **actor_instances;
} ULevelRuntimeConfig;

/**
 * @brief Initializes reusable level runtime state over caller-owned actor/collision/spawn storage.
 * @details No content is loaded until `unsigned_level_load()` is called.
 * @param config Caller-owned composition pointers consumed during initialization.
 * @pre `config` and all runtime/storage pointers it contains are valid for the lifetime of `level`.
 * @pre The combined actor-pool capacity fits `u8`, and `actor_instances` provides that many entries.
 * @pre Spawn/collision storage matches the authored pool/channel capacities.
 */
void unsigned_level_init(ULevel *level, const ULevelRuntimeConfig *config);

/**
 * @brief Updates AI/collision TLSS scales used by the active and future level content.
 * @pre `level` and `tlss_config` are valid and both scales are authored UTLSSScale values.
 */
void unsigned_level_set_tlss_config(ULevel *level, const struct UTLSSScaleConfig *tlss_config);

/**
 * @brief Replace current level content with `definition`, run its authored setup/spawns, and build static collision.
 * @pre `level` and `definition` are valid.
 * @pre The definition spawn counts and collision channels fit the storage/capacities configured for `level`.
 * @pre Optional load/spawn callbacks satisfy their documented dependencies.
 */
void unsigned_level_load(ULevel *level, const ULevelDefinition *definition, void *context);

/**
 * @brief Advances one loaded-level frame using the current input and viewport.
 * @pre `level` is loaded and `viewport` belongs to the active game runtime.
 * @pre `input` is the input manager used by actors in this level.
 */
void unsigned_level_tick(ULevel *level, struct UInputManager *input, const struct UViewport *viewport);

/** Run the definition unload hook, clear gameplay/pools/collision and detach the loaded definition. */
void unsigned_level_unload(ULevel *level);

#endif
