/**
 * @file level_manager.h
 * @brief Graph-driven and directly controlled level loading.
 */

#ifndef UNSIGNED_LEVEL_MANAGER_H
#define UNSIGNED_LEVEL_MANAGER_H

#include "core/state/state_graph.h"
#include "core/state/state_graph_clock.h"
#include "level/level_definition.h"

typedef struct ULevelBinding {
    const UStateGraphNode *node;
    const ULevelDefinition *definition;
} ULevelBinding;

/**
 * @brief Associates authored state-machine phases with the level content loaded for each phase.
 *
 * The graph is immutable content: transitions decide *when* the game changes phase, while bindings
 * decide *which* level definition represents that phase. Keeping this mapping outside the runtime
 * manager lets one state graph drive level changes without repeated structural validation.
 *
 * @invariant `initial != NULL`, `bindings != NULL`, and `binding_count > 0`.
 * @invariant Every state that can become current has exactly one binding with a non-NULL definition.
 * @invariant The state hierarchy rooted at `initial` follows the UStateGraphNode authoring contract.
 */
typedef struct ULevelGraph {
    const UStateGraphNode *global;
    const UStateGraphNode *initial;
    const ULevelBinding *bindings;
    u8 binding_count;
} ULevelGraph;

typedef enum ULevelManagerStatus {
    U_LEVEL_MANAGER_STOPPED = 0,
    U_LEVEL_MANAGER_ACTIVE,
    U_LEVEL_MANAGER_WAITING,
} ULevelManagerStatus;

/** Selects whether scene changes are driven by the internal state graph or directly by the caller. */
typedef enum ULevelManagerMode {
    U_LEVEL_MANAGER_MODE_GRAPH = 0,
    U_LEVEL_MANAGER_MODE_DIRECT,
} ULevelManagerMode;

typedef struct ULevelManager {
    UStateGraph state_graph;
    UStateGraphClock state_graph_clock;
    ULevel *level;
    const ULevelGraph *graph;
    const ULevelBinding *active_binding;
    void *level_context;
    ULevelManagerMode mode;
    ULevelManagerStatus status;
} ULevelManager;

/**
 * @brief Initializes state-graph-driven level transitions and loads the initial level when its enter conditions already pass.
 *
 * @details If the initial state enter condition is currently false, initialization succeeds with U_LEVEL_MANAGER_WAITING and tick() retries entry later.
 *
 * @param manager Level manager runtime state to initialize.
 * @param level Level runtime reused for each bound level definition.
 * @param graph Level graph whose state nodes map to level definitions.
 * @param condition_context Opaque context passed to state-graph conditions.
 * @param level_context Opaque context passed to level load/unload callbacks.
 * @pre `manager`, `level` and `graph` are valid and `graph` satisfies ULevelGraph invariants.
 */
void unsigned_level_manager_init(ULevelManager *manager, ULevel *level, const ULevelGraph *graph, void *condition_context, void *level_context);

/**
 * @brief Initializes a directly controlled level manager without a state graph.
 *
 * @details This mode is intended for callers whose higher-level runtime already owns the
 *          phase/state machine (for example the Neo Geo BIOS workflow). The caller changes
 *          scenes explicitly with unsigned_level_manager_set().
 *
 * @param manager Level manager runtime state to initialize.
 * @param level Level runtime reused for each definition.
 * @param initial Initial level definition to load.
 * @param level_context Opaque context passed to level load/unload callbacks.
 * @pre `manager`, `level` and `initial` are valid.
 */
void unsigned_level_manager_init_direct(ULevelManager *manager, ULevel *level, const ULevelDefinition *initial, void *level_context);

/**
 * @brief Replaces the current level in directly controlled mode.
 *
 * @param manager Directly controlled level manager.
 * @param definition Definition to load.
 * @pre `manager` is a directly controlled manager and `definition` is valid.
 */
void unsigned_level_manager_set(ULevelManager *manager, const ULevelDefinition *definition);

/**
 * @brief Dispatches a state-graph event while active and synchronizes the loaded level with any resulting transition.
 *
 * @param manager Active level manager.
 * @param event State-graph event index to dispatch; ignored while manager is not active.
 */
void unsigned_level_manager_send_event(ULevelManager *manager, UEvent event);

/**
 * @brief Advances pending entry/state transitions and keeps the loaded level synchronized with the active graph binding.
 *
 * @param manager Level manager to advance once per engine frame.
 * @details The resulting lifecycle is stored in `manager->status`; callers that gate scene simulation read that state directly.
 * @pre Every state reachable during this tick has a ULevelGraph binding.
 */
void unsigned_level_manager_tick(ULevelManager *manager);

/**
 * @brief Stops the state graph, unloads the current level and leaves the manager in U_LEVEL_MANAGER_STOPPED.
 *
 * @param manager Level manager to stop.
 * @pre `manager` is valid.
 */
void unsigned_level_manager_stop(ULevelManager *manager);

/**
 * @brief Returns the level definition bound to the currently loaded state.
 *
 * @param manager Level manager to query.
 * @return Current level definition, or NULL when no level is active.
 * @pre `manager` is valid.
 */
const ULevelDefinition *unsigned_level_manager_current(const ULevelManager *manager);

#endif
