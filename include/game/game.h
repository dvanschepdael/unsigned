/**
 * @file game.h
 * @brief Top-level composition root for one running Unsigned game instance.
 */

#ifndef UNSIGNED_GAME_H
#define UNSIGNED_GAME_H

#include "actor/npc.h"
#include "actor/object.h"
#include "audio/audio.h"
#include "core/timer/timer_pool.h"
#include "core/tlss/tlss.h"
#include "gameplay/runtime.h"
#include "input/input.h"
#include "level/level.h"
#include "level/level_manager.h"
#include "level/level_runtime.h"
#include "renderer/level_renderer.h"

/** Caller-owned fixed memory used by a game instance; no field is allocated internally. */
typedef struct UGameInstanceStorage {
    /** `player_capacity` pool slots. */
    UPoolInstance *players;
    /** `npc_capacity` pool slots, or NULL when npc_capacity is zero. */
    UPoolInstance *npcs;
    /** `object_capacity` pool slots, or NULL when object_capacity is zero. */
    UPoolInstance *objects;
    /** `projectile_capacity` pool slots, or NULL when projectile_capacity is zero. */
    UPoolInstance *projectiles;
    /** `npc_capacity` NPC runtime records. */
    UNpc *npc_runtime;
    /** `npc_capacity` character runtime records backing NPCs. */
    UCharacter *npc_character_runtime;
    /** `object_capacity` object runtime records. */
    UObject *object_runtime;
    /** `U_GAME_ACTOR_CAPACITY(...)` entries used to build the level-wide actor view. */
    UActor **actors;
    /** `U_GAME_COLLISION_LAYER_STORAGE_CAPACITY(...)` pointer entries. */
    const UCollisionBox **collision_layer_boxes;
} UGameInstanceStorage;

typedef struct UGameInstanceConfig {
    u8 player_capacity;
    u8 npc_capacity;
    u8 object_capacity;
    u8 projectile_capacity;
    /** Engine ticks per second used by frame-based time conversion; must be non-zero. */
    u8 refresh_rate;
    UTLSSScaleConfig tlss;
    /** All variable-capacity backing memory; ownership remains with the application. */
    UGameInstanceStorage storage;
    /** Optional caller-owned immutable event definitions retained by the audio manager. */
    const UAudioEventDefinition *audio_events;
    /** Optional direct initial level. Use this when level_graph is NULL. */
    const ULevelDefinition *initial_level;
    /** Optional state-graph-driven level flow. Mutually exclusive with initial_level. */
    const ULevelGraph *level_graph;
    /** Opaque context used only by level-graph transition conditions. */
    void *level_condition_context;
    /** Opaque context forwarded to level load/lifecycle callbacks. */
    void *level_context;
} UGameInstanceConfig;

typedef struct UGameInstance {
    UAudioManager audio;
    UInputManager input;
    UTimerPool timers;
    UGameplayRuntime gameplay;
    UActorPoolSet actor_pools;
    ULevel level;
    ULevelRenderer renderer;
    ULevelManager level_manager;
    UViewport viewport;
} UGameInstance;

/**
 * @brief Builds one game runtime from caller-owned fixed storage and authored content.
 *
 * The composition root wires pools, gameplay, collision, rendering, timing, audio and level flow.
 * Configuration is treated as authored data: structural invariants are contracts rather than work
 * repeated at runtime. Initialization therefore performs composition directly instead of reporting
 * configuration mistakes as runtime failures.
 *
 * @param game Runtime instance to initialize.
 * @param config Persistent configuration and backing storage used to compose the runtime.
 * @pre `game` and `config` are valid.
 * @pre Exactly one of `config->initial_level` and `config->level_graph` is non-NULL.
 * @pre `player_capacity` is in [1, U_INPUT_PLAYER_CAPACITY] and `refresh_rate` is non-zero.
 * @pre The sum of actor pool capacities fits in u8.
 * @pre Every non-zero pool capacity has matching storage; actor/collision backing arrays are sized
 *      from the corresponding composition macros and collision configuration.
 * @pre `config->tlss.ai` and `config->tlss.collision` are valid UTLSSScale values.
 */
void unsigned_game_instance_init(UGameInstance *game, const UGameInstanceConfig *config);

/**
 * Replace the current level in direct-level mode.
 * @pre `game` is valid and was configured for direct level selection rather than a level graph.
 * @pre `definition` satisfies the capacities authored in the game configuration.
 */
void unsigned_game_instance_set_level(UGameInstance *game, const ULevelDefinition *definition);

/**
 * Advance one gameplay frame. Timers tick first, the level manager decides whether the
 * active level may tick, then actor/gameplay/viewport work runs and audio resolves last.
 */
void unsigned_game_instance_tick(UGameInstance *game);

/** Prepare the current level render plan during active display without touching VRAM. */
void unsigned_game_instance_prepare_render(UGameInstance *game);

/**
 * Commit the level plan prepared during active display.
 * @pre `game` is valid and `unsigned_game_instance_prepare_render()` was called for this frame.
 */
void unsigned_game_instance_commit_render(UGameInstance *game);

/** Tear down runtime-owned state; backing arrays in `UGameInstanceStorage` remain caller-owned. */
void unsigned_game_instance_destroy(UGameInstance *game);

#endif
