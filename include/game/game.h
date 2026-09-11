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

/**
 * @brief Computes the total actor slots represented by the configured actor pools.
 *
 * @param players Player pool capacity.
 * @param npcs NPC pool capacity.
 * @param objects Object pool capacity.
 * @param projectiles Projectile pool capacity.
 * @return Sum of all four capacities. Values above 255 are returned unchanged so callers can
 *         reject configurations that do not fit the runtime actor index.
 */
u16 unsigned_game_actor_capacity(u8 players, u8 npcs, u8 objects, u8 projectiles);

/**
 * @brief Computes the worst-case collision-layer pointer storage required by the actor pools.
 *
 * @param players Player pool capacity.
 * @param npcs NPC pool capacity.
 * @param objects Object pool capacity.
 * @param projectiles Projectile pool capacity.
 * @return Number of collision-box pointer entries required by all configured collision channels.
 */
u16 unsigned_game_collision_layer_storage_capacity(u8 players, u8 npcs, u8 objects, u8 projectiles);

/**
 * @brief Computes collision-query scratch capacity for the actor pools.
 *
 * @param players Player pool capacity.
 * @param npcs NPC pool capacity.
 * @param objects Object pool capacity.
 * @param projectiles Projectile pool capacity.
 * @return Twice the total actor capacity, capped to the `u8` query-count limit of 255.
 */
u8 unsigned_game_collision_query_storage_capacity(u8 players, u8 npcs, u8 objects, u8 projectiles);

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
    /** `unsigned_game_actor_capacity()` entries used to build the level-wide actor view. */
    UActor **actors;
    /** Must be at least `unsigned_game_actor_capacity()` and normally equals it. */
    u8 actor_capacity;
    /** `unsigned_game_collision_layer_storage_capacity()` pointer entries. */
    const UCollisionBox **collision_layer_boxes;
    /** Number of entries available in collision_layer_boxes. */
    u16 collision_layer_box_capacity;
    /** `unsigned_game_collision_query_storage_capacity()` pointer entries. */
    const UCollisionBox **collision_query_hits;
    /** Number of entries available in collision_query_hits. */
    u8 collision_query_hit_capacity;
} UGameInstanceStorage;

typedef struct UGameInstanceConfig {
    u8 player_capacity;
    u8 npc_capacity;
    u8 object_capacity;
    u8 projectile_capacity;
    /** Engine ticks per second used by frame-based time conversion; validated at initialization. */
    u8 refresh_rate;
    UTLSSScaleConfig tlss;
    /** All variable-capacity backing memory; ownership remains with the application. */
    UGameInstanceStorage storage;
    /** Optional caller-owned immutable catalog retained by the audio manager. */
    const UAudioCatalog *audio_catalog;
    /** Optional direct initial level. Use this when level_graph is NULL. */
    const ULevelDefinition *initial_level;
    /** Optional state-graph-driven level flow. Mutually exclusive with initial_level. */
    const ULevelGraph *level_graph;
    /** Opaque context used only by level-graph transition conditions. */
    void *level_condition_context;
    /** Opaque context forwarded to level load/lifecycle callbacks. */
    void *level_context;
} UGameInstanceConfig;

typedef struct UGameInstanceCapacity {
    u8 players;
    u8 npcs;
    u8 objects;
    u8 projectiles;
    u8 actors;
} UGameInstanceCapacity;

typedef struct UGameInstance {
    UAudioManager audio;
    UInputManager input;
    UTimerPool timers;
    UGameplayRuntime gameplay;
    UActorPoolSet actor_pools;
    UGameInstanceCapacity capacity;
    ULevel level;
    ULevelRenderer renderer;
    ULevelManager level_manager;
    UViewport viewport;
} UGameInstance;

/**
 * Initialize and wire every subsystem using caller-provided fixed storage.
 * Validation is performed before the instance becomes usable; false means the capacity,
 * pointer, level graph, TLSS or refresh-rate contract is invalid.
 */
bool unsigned_game_instance_init(UGameInstance *game, const UGameInstanceConfig *config);

/** Replace the level TLSS scale configuration used to schedule temporally scaled work. */
bool unsigned_game_instance_set_tlss_config(UGameInstance *game, const UTLSSScaleConfig *tlss_config);

/** Configured number of player pool slots, or zero for a NULL instance. */
u8 unsigned_game_instance_player_capacity(const UGameInstance *game);
/** Configured number of NPC pool slots, or zero for a NULL instance. */
u8 unsigned_game_instance_npc_capacity(const UGameInstance *game);
/** Configured number of object pool slots, or zero for a NULL instance. */
u8 unsigned_game_instance_object_capacity(const UGameInstance *game);
/** Configured number of projectile pool slots, or zero for a NULL instance. */
u8 unsigned_game_instance_projectile_capacity(const UGameInstance *game);
/** Total player/NPC/object/projectile capacity in the level-wide actor index. */
u8 unsigned_game_instance_actor_capacity(const UGameInstance *game);

/** Mutable input manager used by platform polling and application UI; NULL for a NULL game. */
UInputManager *unsigned_game_instance_input(UGameInstance *game);
/** Mutable timer pool owned by the game instance; NULL for a NULL game. */
UTimerPool *unsigned_game_instance_timers(UGameInstance *game);
/** Current level definition, or NULL when no level is active. */
const ULevelDefinition *unsigned_game_instance_current_level(const UGameInstance *game);

/** Mutable player pool for spawning/despawning player instances; NULL for a NULL game. */
UPoolInstanceContainer *unsigned_game_instance_player_pool(UGameInstance *game);
/** Mutable NPC pool used by level/game code; NULL for a NULL game. */
UPoolInstanceContainer *unsigned_game_instance_npc_pool(UGameInstance *game);
/** Mutable object pool used by level/game code; NULL for a NULL game. */
UPoolInstanceContainer *unsigned_game_instance_object_pool(UGameInstance *game);
/** Mutable projectile pool used by level/game code; NULL for a NULL game. */
UPoolInstanceContainer *unsigned_game_instance_projectile_pool(UGameInstance *game);

/** Forward a game event to the level state graph; the event is ignored for a NULL game. */
void unsigned_game_instance_send_level_event(UGameInstance *game, UEvent event);

/**
 * Replace the current level when the game was configured with initial_level instead of a level graph.
 * Returns false for graph-driven instances or when the requested level cannot be loaded.
 */
bool unsigned_game_instance_set_level(UGameInstance *game, const ULevelDefinition *definition);

/**
 * Advance one gameplay frame. Timers tick first, the level manager decides whether the
 * active level may tick, then actor/gameplay/viewport work runs and audio resolves last.
 */
void unsigned_game_instance_tick(UGameInstance *game);

/** Render the current level through the game viewport using the level renderer. */
void unsigned_game_instance_render(UGameInstance *game);

/** Tear down runtime-owned state; backing arrays in `UGameInstanceStorage` remain caller-owned. */
void unsigned_game_instance_destroy(UGameInstance *game);

#endif
