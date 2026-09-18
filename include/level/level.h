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
struct UCollisionManagerStorage;
struct ULevelSpawnStorage;
struct UTLSSScaleConfig;
struct UViewport;

/**
 * Initialize reusable level runtime state over caller-owned actor/collision/spawn storage.
 * No level content is loaded until `unsigned_level_load()` succeeds.
 */
bool unsigned_level_init(ULevel *level, struct UActorPoolSet *actor_pools, struct UGameplayRuntime *gameplay, const struct ULevelSpawnStorage *spawn_storage, const struct UCollisionManagerConfig *collision_config, const struct UCollisionManagerStorage *collision_storage,
                         struct UActor **actor_instances, u8 actor_capacity);

/** Update AI/collision TLSS scales used now and by future level loads. */
bool unsigned_level_set_tlss_config(ULevel *level, const struct UTLSSScaleConfig *tlss_config);

/**
 * Replace current level content with `definition`, run its load hook/spawns, and build static collision.
 * Failure rolls back partially loaded gameplay/pool/collision state and leaves no definition loaded.
 */
bool unsigned_level_load(ULevel *level, const ULevelDefinition *definition, void *context);

/** Tick one frame without viewport-aware AI activity classification. */
void unsigned_level_tick(ULevel *level, struct UInputManager *input);

/** Tick one frame while allowing AI/TLSS policy to classify NPCs from the current viewport. */
void unsigned_level_tick_with_viewport(ULevel *level, struct UInputManager *input, const struct UViewport *viewport);

/** Run the definition unload hook, clear gameplay/pools/collision and detach the loaded definition. */
void unsigned_level_unload(ULevel *level);

#endif
