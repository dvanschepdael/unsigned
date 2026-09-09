/**
 * @file level_manager.c
 * @brief Implements graph-driven or directly controlled level loading and transitions.
 */

#include "level/level_manager.h"

#include "level/level.h"

/** Returns the state-graph binding associated with the requested level definition. */
static const ULevelBinding *level_manager_binding(const ULevelManager *manager, const UStateGraphNode *node) {
    for (u8 i = 0; i < manager->graph->binding_count; ++i) {
        if (manager->graph->bindings[i].node == node) {
            return &manager->graph->bindings[i];
        }
    }
    return NULL;
}

/** Synchronizes the loaded level with the level-manager state graph after transitions. */
static bool level_manager_sync(ULevelManager *manager) {
    if (manager->state_graph.current == NULL) {
        unsigned_level_unload(manager->level);
        manager->active_binding = NULL;
        manager->current_definition = NULL;
        manager->status = U_LEVEL_MANAGER_STOPPED;
        return true;
    }

    const ULevelBinding *binding = level_manager_binding(manager, manager->state_graph.current);
    if (binding == NULL || binding->definition == NULL) {
        manager->status = U_LEVEL_MANAGER_INVALID_GRAPH;
        return false;
    }
    if (binding == manager->active_binding) {
        return true;
    }

    if (!unsigned_level_load(manager->level, binding->definition, manager->level_context)) {
        manager->active_binding = NULL;
        manager->current_definition = NULL;
        manager->status = U_LEVEL_MANAGER_LOAD_FAILED;
        return false;
    }

    manager->active_binding = binding;
    manager->current_definition = binding->definition;
    manager->status = U_LEVEL_MANAGER_ACTIVE;
    return true;
}

bool unsigned_level_manager_init(ULevelManager *manager, ULevel *level, const ULevelGraph *graph, void *condition_context, void *level_context) {
    if (manager == NULL || level == NULL || graph == NULL || graph->initial == NULL || graph->bindings == NULL || graph->binding_count == 0u) {
        if (manager != NULL) {
            manager->status = U_LEVEL_MANAGER_INVALID_GRAPH;
        }
        return false;
    }

    *manager = (ULevelManager){
        .level = level,
        .graph = graph,
        .level_context = level_context,
        .status = U_LEVEL_MANAGER_STOPPED,
    };
    if (!unsigned_state_graph_init(&manager->state_graph, graph->global, graph->initial, condition_context)) {
        manager->status = U_LEVEL_MANAGER_INVALID_GRAPH;
        return false;
    }

    const ULevelBinding *initial_binding = level_manager_binding(manager, graph->initial);
    if (initial_binding == NULL || initial_binding->definition == NULL) {
        manager->status = U_LEVEL_MANAGER_INVALID_GRAPH;
        return false;
    }

    if (manager->state_graph.current == NULL) {
        manager->status = U_LEVEL_MANAGER_WAITING;
        return true;
    }

    return level_manager_sync(manager);
}

bool unsigned_level_manager_init_direct(ULevelManager *manager, ULevel *level, const ULevelDefinition *initial, void *level_context) {
    if (manager == NULL || level == NULL || initial == NULL) {
        if (manager != NULL) {
            manager->status = U_LEVEL_MANAGER_INVALID_GRAPH;
        }
        return false;
    }

    *manager = (ULevelManager){
        .level = level,
        .level_context = level_context,
        .status = U_LEVEL_MANAGER_STOPPED,
    };

    return unsigned_level_manager_set(manager, initial);
}

bool unsigned_level_manager_set(ULevelManager *manager, const ULevelDefinition *definition) {
    if (manager == NULL || manager->level == NULL || manager->graph != NULL || definition == NULL) {
        return false;
    }
    if (manager->current_definition == definition && manager->status == U_LEVEL_MANAGER_ACTIVE) {
        return true;
    }

    if (!unsigned_level_load(manager->level, definition, manager->level_context)) {
        manager->active_binding = NULL;
        manager->current_definition = NULL;
        manager->status = U_LEVEL_MANAGER_LOAD_FAILED;
        return false;
    }

    manager->active_binding = NULL;
    manager->current_definition = definition;
    manager->status = U_LEVEL_MANAGER_ACTIVE;
    return true;
}

void unsigned_level_manager_send_event(ULevelManager *manager, UEvent event) {
    if (manager == NULL || manager->graph == NULL || manager->status != U_LEVEL_MANAGER_ACTIVE) {
        return;
    }
    unsigned_state_graph_send_event(&manager->state_graph, event);
    (void)level_manager_sync(manager);
}

bool unsigned_level_manager_tick(ULevelManager *manager) {
    if (manager == NULL) {
        return false;
    }

    if (manager->graph == NULL) {
        return manager->status == U_LEVEL_MANAGER_ACTIVE;
    }

    if (manager->status == U_LEVEL_MANAGER_WAITING) {
        if (!unsigned_state_graph_try_start(&manager->state_graph)) {
            return false;
        }
        if (!level_manager_sync(manager)) {
            return false;
        }
    }

    if (manager->status != U_LEVEL_MANAGER_ACTIVE) {
        return false;
    }

    unsigned_state_graph_tick(&manager->state_graph);
    if (!level_manager_sync(manager)) {
        return false;
    }

    return manager->status == U_LEVEL_MANAGER_ACTIVE;
}

void unsigned_level_manager_stop(ULevelManager *manager) {
    if (manager == NULL) {
        return;
    }

    if (manager->graph != NULL) {
        unsigned_state_graph_stop(&manager->state_graph);
        (void)level_manager_sync(manager);
    } else if (manager->level != NULL) {
        unsigned_level_unload(manager->level);
        manager->current_definition = NULL;
    }
    manager->status = U_LEVEL_MANAGER_STOPPED;
}

const ULevelDefinition *unsigned_level_manager_current(const ULevelManager *manager) {
    return manager != NULL ? manager->current_definition : NULL;
}
