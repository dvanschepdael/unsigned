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

typedef enum UTransitionTrigger { U_TRANSITION_ON_SUCCESS = 0, U_TRANSITION_ON_FAILED, U_TRANSITION_ON_COMPLETED, U_TRANSITION_ON_EVENT, U_TRANSITION_COUNT } UTransitionTrigger;

typedef UTaskState (*UStateGraphTask)(UStateGraph *graph, void *context);
typedef void (*UStateGraphCallback)(UStateGraph *graph, void *context);
typedef bool (*UStateGraphCondition)(UStateGraph *graph, void *context);

typedef struct UStateGraphTransition {
    UStateGraphCondition condition;
    const UStateGraphNode *target;
} UStateGraphTransition;

typedef struct UStateGraphTransitionContainer {
    u8 count;
    u8 capacity;
    const UStateGraphTransition *instances;
} UStateGraphTransitionContainer;

typedef struct UStateGraphTaskContainer {
    u8 count;
    u8 capacity;
    const UStateGraphTask *instances;
} UStateGraphTaskContainer;

typedef struct UStateGraphNodeContainer {
    u8 count;
    u8 capacity;
    const UStateGraphNode *instances;
} UStateGraphNodeContainer;

/* Definitions and containers must remain unchanged while a graph uses them.
 * The root parent is NULL; each child parent must match its owning edges node. */
struct UStateGraphNode {
    const UStateGraphNode *parent;
    const UStateGraphNodeContainer *edges;
    UStateGraphCondition enter_condition;
    UStateGraphCallback enter;
    UStateGraphCallback exit;
    UStateGraphTaskContainer tasks;
    const UStateGraphTransitionContainer *transitions[U_TRANSITION_COUNT];
};

struct UStateGraph {
    void *context;
    const UStateGraphNode *global;
    const UStateGraphNode *initial;
    const UStateGraphNode *current;
};

/**
 * @brief Validates the bounded node hierarchy and initializes a state graph.
 *
 * @details Initialization attempts to enter initial immediately. A false initial enter condition leaves the graph valid but not started, allowing a later
 * unsigned_state_graph_try_start(). Invalid structure leaves the graph zeroed and inert.
 *
 * @param graph Runtime graph to initialize.
 * @param global Optional global/root node ticked alongside the active state.
 * @param initial Required initial state node; it must be reachable from the root (global when present, otherwise initial).
 * @param context Opaque caller context passed to conditions, tasks and enter/exit callbacks.
 * @return true when node/container bounds and reachability are valid; false on invalid structure or capacity overflow.
 */
bool unsigned_state_graph_init(UStateGraph *graph, const UStateGraphNode *global, const UStateGraphNode *initial, void *context);

/**
 * @brief Attempts to enter the initial state of an already validated graph.
 *
 * @param graph Initialized state graph that is not currently in a state.
 * @return true when all initial/ancestor enter conditions allow entry; false when graph is invalid, already started or entry is denied.
 */
bool unsigned_state_graph_try_start(UStateGraph *graph);

/**
 * @brief Dispatches an event transition from the active node, falling back through ancestors.
 *
 * @param graph Initialized graph whose current state may change.
 * @param event Zero-based event index used in U_TRANSITION_ON_EVENT transition containers.
 */
void unsigned_state_graph_send_event(UStateGraph *graph, UEvent event);

/**
 * @brief Ticks global and active-state tasks once and applies success/failed/completed transitions.
 *
 * @param graph Initialized graph to advance by one engine frame.
 */
void unsigned_state_graph_tick(UStateGraph *graph);

/**
 * @brief Exits the active state and leaves the validated graph stopped but reusable.
 *
 * @param graph Initialized graph to stop; invalid/already stopped graphs are ignored.
 */
void unsigned_state_graph_stop(UStateGraph *graph);

#endif
