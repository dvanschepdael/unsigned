/**
 * @file state_graph.c
 * @brief Implements hierarchical state-graph execution.
 */

#include "core/state/state_graph.h"

/** Accepts entry only when the node and every ancestor satisfy their authored enter conditions. */
static bool can_enter(UStateGraph *graph, const UStateGraphNode *node) {
    if (node->parent != NULL && !can_enter(graph, node->parent)) {
        return false;
    }
    return node->enter_condition == NULL || node->enter_condition(graph, graph->context);
}

/** Evaluates the optional business guard attached to a transition. */
static bool can_transit(UStateGraph *graph, const UStateGraphTransition *transition) {
    return transition->condition == NULL || transition->condition(graph, graph->context);
}

/** Resolves the first permitted transition for a trigger, walking from the active node toward its ancestors. */
static const UStateGraphNode *get_transition(UStateGraph *graph, const UStateGraphNode *node, UTransitionTrigger trigger) {
    for (const UStateGraphNode *current = node; current != NULL; current = current->parent) {
        const UStateGraphTransitionContainer *transitions = current->transitions[trigger];
        if (transitions == NULL) {
            continue;
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

/** Exits the previous state, switches the active node and enters the target when one exists. */
static void change_state(UStateGraph *graph, const UStateGraphNode *target) {
    if (graph->current != NULL && graph->current->exit != NULL) {
        graph->current->exit(graph, graph->context);
    }

    graph->current = target;

    if (target != NULL && target->enter != NULL) {
        target->enter(graph, graph->context);
    }
}

void unsigned_state_graph_try_start(UStateGraph *graph) {
    if (can_enter(graph, graph->initial)) {
        change_state(graph, graph->initial);
    }
}

void unsigned_state_graph_init(UStateGraph *graph, const UStateGraphNode *global, const UStateGraphNode *initial, void *context) {
    *graph = (UStateGraph){
        .context = context,
        .global = global,
        .initial = initial,
    };
    unsigned_state_graph_try_start(graph);
}

/** Resolves an event-indexed transition from the active state or the nearest ancestor that handles it. */
static const UStateGraphNode *get_event_target(UStateGraph *graph, const UStateGraphNode *node, UEvent event) {
    for (const UStateGraphNode *current = node; current != NULL; current = current->parent) {
        const UStateGraphTransitionContainer *container = current->transitions[U_TRANSITION_ON_EVENT];
        if (container == NULL || event >= container->count) {
            continue;
        }

        const UStateGraphTransition *transition = &container->instances[event];
        if (transition->target != NULL && can_transit(graph, transition) && can_enter(graph, transition->target)) {
            return transition->target;
        }
    }
    return NULL;
}

void unsigned_state_graph_send_event(UStateGraph *graph, UEvent event) {
    const UStateGraphNode *target = get_event_target(graph, graph->current, event);
    if (target != NULL) {
        change_state(graph, target);
    }
}

/** Runs one node task set and converts its first terminal result into authored transitions. */
static void tick_node(UStateGraph *graph, const UStateGraphNode *node) {
    if (node->tasks.count == 0u) {
        return;
    }

    UTaskState state = U_TASK_RUNNING;
    for (u8 i = 0u; i < node->tasks.count; ++i) {
        state = node->tasks.instances[i](graph, graph->context);
        if (state != U_TASK_RUNNING) {
            break;
        }
    }
    if (state == U_TASK_RUNNING) {
        return;
    }

    UTransitionTrigger trigger;
    switch (state) {
    case U_TASK_SUCCESS:
        trigger = U_TRANSITION_ON_SUCCESS;
        break;
    case U_TASK_FAILED:
        trigger = U_TRANSITION_ON_FAILED;
        break;
    case U_TASK_RUNNING:
    default:
        U_UNREACHABLE();
    }

    const UStateGraphNode *target = get_transition(graph, node, trigger);
    if (target == NULL) {
        target = get_transition(graph, node, U_TRANSITION_ON_COMPLETED);
    }

    if (target != NULL) {
        change_state(graph, target);
    } else if (node != graph->global) {
        change_state(graph, NULL);
    }
}

void unsigned_state_graph_tick(UStateGraph *graph) {
    if (graph->global != NULL) {
        tick_node(graph, graph->global);
    }

    const UStateGraphNode *active = graph->current;
    if (active != NULL && active != graph->global) {
        tick_node(graph, active);
    }
}

void unsigned_state_graph_timeout(UStateGraph *graph) {
    if (graph->current == NULL) {
        return;
    }

    const UStateGraphNode *target = get_transition(graph, graph->current, U_TRANSITION_ON_TIMEOUT);
    if (target != NULL) {
        change_state(graph, target);
    }
}

void unsigned_state_graph_stop(UStateGraph *graph) {
    if (graph->current != NULL) {
        change_state(graph, NULL);
    }
}
