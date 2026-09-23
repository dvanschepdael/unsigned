/**
 * @file state_graph.h
 * @brief Hierarchical state-graph execution.
 */

#ifndef UNSIGNED_CORE_STATE_GRAPH_H
#define UNSIGNED_CORE_STATE_GRAPH_H

#include "core/state/config.h"
#include "core/types.h"

typedef struct UStateGraph UStateGraph;
typedef struct UStateGraphNode UStateGraphNode;

typedef enum UTaskState { U_TASK_RUNNING = 0, U_TASK_SUCCESS, U_TASK_FAILED } UTaskState;

typedef enum UTransitionTrigger { U_TRANSITION_ON_SUCCESS = 0, U_TRANSITION_ON_FAILED, U_TRANSITION_ON_COMPLETED, U_TRANSITION_ON_EVENT, U_TRANSITION_ON_TIMEOUT, U_TRANSITION_COUNT } UTransitionTrigger;

typedef UTaskState (*UStateGraphTask)(UStateGraph *graph, void *context);
typedef void (*UStateGraphCallback)(UStateGraph *graph, void *context);
typedef bool (*UStateGraphCondition)(UStateGraph *graph, void *context);

typedef struct UStateGraphTransition {
    UStateGraphCondition condition;
    const UStateGraphNode *target;
} UStateGraphTransition;

typedef struct UStateGraphTransitionContainer {
    u8 count;
    const UStateGraphTransition *instances;
} UStateGraphTransitionContainer;

typedef struct UStateGraphTaskContainer {
    u8 count;
    const UStateGraphTask *instances;
} UStateGraphTaskContainer;

/**
 * @brief Immutable authored state node used by UStateGraph.
 *
 * State definitions are content: they are built once, then shared by the runtime. Parent links express
 * fallback/ancestry, while transition arrays map completion, events and timeouts to candidate target states.
 *
 * @invariant Definitions and their containers remain unchanged while a graph uses them.
 * @invariant The root parent is NULL and every child parent points to its authored ancestor.
 * @invariant Non-empty task/transition containers provide backing instances.
 * @invariant Task callbacks return one of the declared `UTaskState` values.
 * @invariant Transition targets belong to the same authored hierarchy.
 */
struct UStateGraphNode {
    const UStateGraphNode *parent;
    UStateGraphCondition enter_condition;
    UStateGraphCallback enter;
    UStateGraphCallback exit;
    UStateGraphTaskContainer tasks;
    /** Optional frame duration before U_TRANSITION_ON_TIMEOUT is evaluated; zero disables timeout. */
    u16 duration_frames;
    const UStateGraphTransitionContainer *transitions[U_TRANSITION_COUNT];
};

struct UStateGraph {
    void *context;
    const UStateGraphNode *global;
    const UStateGraphNode *initial;
    const UStateGraphNode *current;
};

/**
 * @brief Binds an authored state hierarchy to runtime context and attempts its initial entry.
 *
 * A denied initial enter condition is not an initialization failure: the graph stays stopped and can
 * later be started with unsigned_state_graph_try_start(). Structural correctness belongs to content
 * authoring, so initialization does not walk the hierarchy merely to re-check those invariants.
 *
 * @param graph Runtime graph to initialize.
 * @param global Optional global/root node ticked alongside the active state.
 * @param initial Initial state entered when its own and its ancestors' conditions allow it.
 * @param context Opaque caller context passed to conditions, tasks and lifecycle callbacks.
 * @pre `graph` and `initial` are valid.
 * @pre `initial` belongs to the hierarchy rooted at `global` when `global` is non-NULL.
 * @pre The hierarchy satisfies the invariants documented by UStateGraphNode.
 */
void unsigned_state_graph_init(UStateGraph *graph, const UStateGraphNode *global, const UStateGraphNode *initial, void *context);

/**
 * @brief Attempts to enter the initial state of a stopped graph.
 *
 * @param graph Initialized state graph that is not currently in a state.
 * @details When authored enter conditions deny entry, the graph remains stopped (`current == NULL`).
 * @pre `graph` is initialized and currently stopped.
 */
void unsigned_state_graph_try_start(UStateGraph *graph);

/**
 * @brief Dispatches an event transition from the active node, falling back through ancestors.
 *
 * @param graph Initialized graph whose current state may change.
 * @param event Zero-based event index used in U_TRANSITION_ON_EVENT transition containers.
 * @pre `graph` is valid; sending an event while stopped is a no-op.
 */
void unsigned_state_graph_send_event(UStateGraph *graph, UEvent event);

/**
 * @brief Ticks global and active-state tasks once and applies task-driven transitions.
 *
 * @param graph Initialized graph to advance by one engine frame.
 * @pre `graph` is valid.
 */
void unsigned_state_graph_tick(UStateGraph *graph);

/**
 * @brief Applies the first permitted timeout transition from the active node/ancestors.
 *
 * @details Timing is deliberately external to UStateGraph. Callers such as UStateGraphClock
 *          signal expiry through this function after owning/advancing their temporal state.
 *
 * @details When no timeout transition is permitted, `graph->current` is left unchanged.
 * @pre `graph` is valid.
 */
void unsigned_state_graph_timeout(UStateGraph *graph);

/**
 * @brief Exits the active state and leaves the graph stopped but reusable.
 *
 * @param graph Initialized graph to stop; an already stopped graph is unchanged.
 * @pre `graph` is valid.
 */
void unsigned_state_graph_stop(UStateGraph *graph);

#endif
