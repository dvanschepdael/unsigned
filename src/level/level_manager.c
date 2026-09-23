/**
 * @file level_manager.c
 * @brief Implements graph-driven or directly controlled level loading and transitions.
 */

#include "level/level_manager.h"

#include "level/level.h"
#include "level/level_runtime.h"

/**
 * @brief Resolves the authored level binding for a state reached by the graph.
 * @pre `node` has exactly one binding in `manager->graph`.
 */
static const ULevelBinding *level_manager_binding(const ULevelManager *manager, const UStateGraphNode *node) {
    for (u8 i = 0; i < manager->graph->binding_count; ++i) {
        if (manager->graph->bindings[i].node == node) {
            return &manager->graph->bindings[i];
        }
    }
    U_UNREACHABLE();
}

/** Synchronizes the loaded level with the level-manager state graph after transitions. */
static void level_manager_sync(ULevelManager *manager) {
    if (manager->state_graph.current == NULL) {
        unsigned_level_unload(manager->level);
        manager->active_binding = NULL;
        manager->status = U_LEVEL_MANAGER_STOPPED;
        return;
    }

    /* Most frames stay in the same graph state. The active binding is therefore also a cache key,
     * avoiding a linear binding lookup until the state actually changes. */
    if (manager->active_binding != NULL && manager->active_binding->node == manager->state_graph.current) {
        return;
    }

    const ULevelBinding *binding = level_manager_binding(manager, manager->state_graph.current);
    unsigned_level_load(manager->level, binding->definition, manager->level_context);
    manager->active_binding = binding;
    manager->status = U_LEVEL_MANAGER_ACTIVE;
}

void unsigned_level_manager_init(ULevelManager *manager, ULevel *level, const ULevelGraph *graph, void *condition_context, void *level_context) {
    *manager = (ULevelManager){
        .level = level,
        .graph = graph,
        .level_context = level_context,
        .mode = U_LEVEL_MANAGER_MODE_GRAPH,
        .status = U_LEVEL_MANAGER_STOPPED,
    };
    unsigned_state_graph_init(&manager->state_graph, graph->global, graph->initial, condition_context);
    unsigned_state_graph_clock_reset(&manager->state_graph_clock, &manager->state_graph);

    if (manager->state_graph.current == NULL) {
        manager->status = U_LEVEL_MANAGER_WAITING;
        return;
    }

    level_manager_sync(manager);
}

void unsigned_level_manager_init_direct(ULevelManager *manager, ULevel *level, const ULevelDefinition *initial, void *level_context) {
    *manager = (ULevelManager){
        .level = level,
        .level_context = level_context,
        .mode = U_LEVEL_MANAGER_MODE_DIRECT,
        .status = U_LEVEL_MANAGER_STOPPED,
    };

    unsigned_level_manager_set(manager, initial);
}

void unsigned_level_manager_set(ULevelManager *manager, const ULevelDefinition *definition) {
    if (manager->level->definition == definition && manager->status == U_LEVEL_MANAGER_ACTIVE) {
        return;
    }

    unsigned_level_load(manager->level, definition, manager->level_context);
    manager->active_binding = NULL;
    manager->status = U_LEVEL_MANAGER_ACTIVE;
}

void unsigned_level_manager_send_event(ULevelManager *manager, UEvent event) {
    if (manager->mode == U_LEVEL_MANAGER_MODE_DIRECT || manager->status != U_LEVEL_MANAGER_ACTIVE) {
        return;
    }
    unsigned_state_graph_send_event(&manager->state_graph, event);
    level_manager_sync(manager);
}

void unsigned_level_manager_tick(ULevelManager *manager) {
    if (manager->mode == U_LEVEL_MANAGER_MODE_DIRECT) {
        return;
    }

    if (manager->status == U_LEVEL_MANAGER_WAITING) {
        unsigned_state_graph_try_start(&manager->state_graph);
        if (manager->state_graph.current == NULL) {
            return;
        }
        unsigned_state_graph_clock_reset(&manager->state_graph_clock, &manager->state_graph);
        level_manager_sync(manager);
    }

    if (manager->status != U_LEVEL_MANAGER_ACTIVE) {
        return;
    }

    unsigned_state_graph_clock_tick(&manager->state_graph_clock, &manager->state_graph);
    level_manager_sync(manager);
}

void unsigned_level_manager_stop(ULevelManager *manager) {
    if (manager->mode == U_LEVEL_MANAGER_MODE_GRAPH) {
        unsigned_state_graph_stop(&manager->state_graph);
        level_manager_sync(manager);
    } else {
        unsigned_level_unload(manager->level);
    }
    manager->status = U_LEVEL_MANAGER_STOPPED;
}

const ULevelDefinition *unsigned_level_manager_current(const ULevelManager *manager) {
    return manager->level->definition;
}
