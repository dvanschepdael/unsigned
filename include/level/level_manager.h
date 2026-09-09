/**
 * @file level_manager.h
 * @brief Graph-driven and directly controlled level loading.
 */

#ifndef UNSIGNED_LEVEL_MANAGER_H
#define UNSIGNED_LEVEL_MANAGER_H

#include "core/state/state_graph.h"
#include "level/level_definition.h"

typedef struct ULevelBinding {
    const UStateGraphNode *node;
    const ULevelDefinition *definition;
} ULevelBinding;

typedef struct ULevelGraph {
    const UStateGraphNode *global;
    const UStateGraphNode *initial;
    const ULevelBinding *bindings;
    u8 binding_count;
} ULevelGraph;

typedef enum ULevelManagerStatus {
    U_LEVEL_MANAGER_STOPPED = 0,
    U_LEVEL_MANAGER_ACTIVE,
    U_LEVEL_MANAGER_LOAD_FAILED,
    U_LEVEL_MANAGER_INVALID_GRAPH,
    U_LEVEL_MANAGER_WAITING,
} ULevelManagerStatus;

typedef struct ULevelManager {
    UStateGraph state_graph;
    ULevel *level;
    const ULevelGraph *graph;
    const ULevelBinding *active_binding;
    const ULevelDefinition *current_definition;
    void *level_context;
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
 * @return true when graph/bindings are valid and any immediate initial load succeeds; false on invalid graph or load failure.
 */
bool unsigned_level_manager_init(ULevelManager *manager, ULevel *level, const ULevelGraph *graph, void *condition_context, void *level_context);

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
 * @return true when the initial level loads successfully; false otherwise.
 */
bool unsigned_level_manager_init_direct(ULevelManager *manager, ULevel *level, const ULevelDefinition *initial, void *level_context);

/**
 * @brief Replaces the current level in directly controlled mode.
 *
 * @param manager Directly controlled level manager.
 * @param definition Definition to load.
 * @return true when the requested level is active; false on invalid mode or load failure.
 */
bool unsigned_level_manager_set(ULevelManager *manager, const ULevelDefinition *definition);

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
 * @return true while the manager ends the tick in U_LEVEL_MANAGER_ACTIVE; false while waiting/stopped or after invalid-graph/load failure.
 */
bool unsigned_level_manager_tick(ULevelManager *manager);

/**
 * @brief Stops the state graph, unloads the current level and leaves the manager in U_LEVEL_MANAGER_STOPPED.
 *
 * @param manager Level manager to stop; NULL is ignored.
 */
void unsigned_level_manager_stop(ULevelManager *manager);

/**
 * @brief Returns the level definition bound to the currently loaded state.
 *
 * @param manager Level manager to query.
 * @return Current level definition, or NULL when no binding is active.
 */
const ULevelDefinition *unsigned_level_manager_current(const ULevelManager *manager);

#endif
