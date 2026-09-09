/**
 * @file state_graph.c
 * @brief Implements hierarchical state-graph execution.
 */

#include "core/state/state_graph.h"

/** Validates parent links and container bounds before traversing transition targets. */
static bool validate_hierarchy(const UStateGraphNode *node, const UStateGraphNode *parent, u8 *node_count) {
    if (node == NULL || node->parent != parent || *node_count >= UNSIGNED_STATE_GRAPH_MAX_NODES) {
        return false;
    }
    ++*node_count;

    if (node->tasks.count > node->tasks.capacity || (node->tasks.count > 0u && node->tasks.instances == NULL)) {
        return false;
    }

    for (u8 trigger = 0u; trigger < U_TRANSITION_COUNT; ++trigger) {
        const UStateGraphTransitionContainer *transitions = node->transitions[trigger];
        if (transitions != NULL && (transitions->count > transitions->capacity || (transitions->count > 0u && transitions->instances == NULL))) {
            return false;
        }
    }

    const UStateGraphNodeContainer *edges = node->edges;
    if (edges == NULL) {
        return true;
    }
    if (edges->count > edges->capacity || (edges->count > 0u && edges->instances == NULL)) {
        return false;
    }
    for (u8 i = 0u; i < edges->count; ++i) {
        if (!validate_hierarchy(&edges->instances[i], node, node_count)) {
            return false;
        }
    }
    return true;
}

/** Finds a node in the already validated hierarchy, only during initialization. */
static bool contains_node(const UStateGraphNode *root, const UStateGraphNode *node) {
    if (root == node) {
        return true;
    }
    if (root->edges != NULL) {
        for (u8 i = 0u; i < root->edges->count; ++i) {
            if (contains_node(&root->edges->instances[i], node)) {
                return true;
            }
        }
    }
    return false;
}

/** Rejects transition targets outside the validated hierarchy. */
static bool validate_targets(const UStateGraphNode *root, const UStateGraphNode *node) {
    for (u8 trigger = 0u; trigger < U_TRANSITION_COUNT; ++trigger) {
        const UStateGraphTransitionContainer *transitions = node->transitions[trigger];
        if (transitions == NULL) {
            continue;
        }
        for (u8 i = 0u; i < transitions->count; ++i) {
            const UStateGraphNode *target = transitions->instances[i].target;
            if (target != NULL && !contains_node(root, target)) {
                return false;
            }
        }
    }
    if (node->edges != NULL) {
        for (u8 i = 0u; i < node->edges->count; ++i) {
            if (!validate_targets(root, &node->edges->instances[i])) {
                return false;
            }
        }
    }
    return true;
}

/** Accepts entry only when the node and every ancestor satisfy their enter conditions. */
static bool can_enter(UStateGraph *graph, const UStateGraphNode *node) {
    if (node == NULL) {
        return false;
    }

    const UStateGraphNode *parent = node->parent;
    if (parent != NULL && !can_enter(graph, parent)) {
        return false;
    }

    return node->enter_condition == NULL || node->enter_condition(graph, graph->context);
}

/** Evaluates the optional guard attached to a candidate state transition. */
static bool can_transit(UStateGraph *graph, const UStateGraphTransition *transition) {
    if (transition == NULL) {
        return false;
    }

    return transition->condition == NULL || transition->condition(graph, graph->context);
}

/** Resolves the first permitted transition for a trigger, walking from the active node toward its ancestors. */
static const UStateGraphNode *get_transition(UStateGraph *graph, const UStateGraphNode *node, UTransitionTrigger trigger) {
    const UStateGraphNode *current;

    if (graph == NULL || graph->initial == NULL || node == NULL || trigger >= U_TRANSITION_COUNT) {
        return NULL;
    }

    for (current = node; current != NULL; current = current->parent) {
        const UStateGraphTransitionContainer *transitions = current->transitions[trigger];

        if (transitions == NULL || transitions->count == 0u) {
            continue;
        }

        if (transitions->instances == NULL || transitions->count > transitions->capacity) {
            return NULL;
        }

        for (u8 i = 0u; i < transitions->count; ++i) {
            const UStateGraphTransition *transition = &transitions->instances[i];

            if (can_transit(graph, transition) && can_enter(graph, transition->target)) {
                return transition->target;
            }
        }
    }

    return NULL;
}

/** Invokes the current node exit callback, switches current, then invokes the target enter callback. */
static void change_state(UStateGraph *graph, const UStateGraphNode *target) {
    if (graph->current != NULL && graph->current->exit != NULL) {
        graph->current->exit(graph, graph->context);
    }

    graph->current = target;

    if (graph->current != NULL && graph->current->enter != NULL) {
        graph->current->enter(graph, graph->context);
    }
}

bool unsigned_state_graph_try_start(UStateGraph *graph) {
    if (graph == NULL || graph->initial == NULL || graph->current != NULL) {
        return false;
    }

    if (!can_enter(graph, graph->initial)) {
        return false;
    }

    change_state(graph, graph->initial);
    return true;
}

bool unsigned_state_graph_init(UStateGraph *graph, const UStateGraphNode *global, const UStateGraphNode *initial, void *context) {
    if (graph == NULL) {
        return false;
    }

    *graph = (UStateGraph){ 0 };
    const UStateGraphNode *root = global != NULL ? global : initial;
    u8 node_count = 0u;
    if (initial == NULL || !validate_hierarchy(root, NULL, &node_count) || !contains_node(root, initial) || !validate_targets(root, root)) {
        return false;
    }

    *graph = (UStateGraph){
        .context = context,
        .global = global,
        .initial = initial,
    };
    (void)unsigned_state_graph_try_start(graph);
    return true;
}

/** Resolves an event-indexed transition target from the active node or the nearest ancestor that handles it. */
static const UStateGraphNode *get_event_target(UStateGraph *graph, const UStateGraphNode *node, UEvent event) {
    const UStateGraphNode *current;

    if (graph == NULL || graph->initial == NULL || node == NULL) {
        return NULL;
    }

    for (current = node; current != NULL; current = current->parent) {
        const UStateGraphTransitionContainer *container = current->transitions[U_TRANSITION_ON_EVENT];

        if (container == NULL || container->count == 0u || event >= container->count) {
            continue;
        }

        if (container->instances == NULL || container->count > container->capacity) {
            return NULL;
        }

        const UStateGraphTransition *transition = &container->instances[event];
        if (transition->target != NULL && can_transit(graph, transition) && can_enter(graph, transition->target)) {
            return transition->target;
        }
    }

    return NULL;
}

void unsigned_state_graph_send_event(UStateGraph *graph, UEvent event) {
    if (graph == NULL || graph->initial == NULL) {
        return;
    }

    const UStateGraphNode *target = get_event_target(graph, graph->current, event);
    if (target != NULL) {
        change_state(graph, target);
    }
}

/** Runs node tasks in declaration order and stops at the first task that completes or fails. */
static UTaskState tick_tasks(UStateGraph *graph, const UStateGraphTaskContainer *tasks) {
    if (tasks == NULL || tasks->count == 0u) {
        return U_TASK_RUNNING;
    }

    if (tasks->instances == NULL || tasks->count > tasks->capacity) {
        return U_TASK_RUNNING;
    }

    for (u8 i = 0u; i < tasks->count; ++i) {
        UStateGraphTask task = tasks->instances[i];

        if (task == NULL) {
            continue;
        }

        UTaskState state = task(graph, graph->context);
        if (state != U_TASK_RUNNING) {
            return state;
        }
    }

    return U_TASK_RUNNING;
}

/** Runs one node task set and converts its terminal result into success/failed/completed transitions. */
static void tick_node(UStateGraph *graph, const UStateGraphNode *node) {
    if (node == NULL || node->tasks.count == 0u) {
        return;
    }

    UTaskState state = tick_tasks(graph, &node->tasks);
    if (state == U_TASK_RUNNING) {
        return;
    }

    UTransitionTrigger trigger = state == U_TASK_SUCCESS ? U_TRANSITION_ON_SUCCESS : U_TRANSITION_ON_FAILED;
    const UStateGraphNode *target = get_transition(graph, node, trigger);

    if (target == NULL) {
        target = get_transition(graph, node, U_TRANSITION_ON_COMPLETED);
    }

    if (target != NULL) {
        change_state(graph, target);
        return;
    }

    if (node != graph->global) {
        change_state(graph, NULL);
    }
}

void unsigned_state_graph_tick(UStateGraph *graph) {
    if (graph == NULL || graph->initial == NULL) {
        return;
    }

    if (graph->global != NULL) {
        tick_node(graph, graph->global);
    }

    if (graph->current != NULL && graph->current != graph->global) {
        tick_node(graph, graph->current);
    }
}

void unsigned_state_graph_stop(UStateGraph *graph) {
    if (graph == NULL || graph->initial == NULL || graph->current == NULL) {
        return;
    }

    change_state(graph, NULL);
}
