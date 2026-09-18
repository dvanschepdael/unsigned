/**
 * @file state_graph_clock.c
 * @brief Implements optional temporal execution for state graphs.
 */

#include "core/state/state_graph_clock.h"

void unsigned_state_graph_clock_reset(UStateGraphClock *clock, const UStateGraph *graph) {
    if (clock == NULL) {
        return;
    }

    clock->state = graph != NULL ? graph->current : NULL;
    clock->elapsed_frames = 0u;
}

void unsigned_state_graph_clock_tick(UStateGraphClock *clock, UStateGraph *graph) {
    if (graph == NULL) {
        return;
    }

    const UStateGraphNode *active = graph->current;
    unsigned_state_graph_tick(graph);

    if (clock == NULL) {
        return;
    }

    /* Task/global transitions do not charge a frame to the newly entered state. */
    if (graph->current != active) {
        unsigned_state_graph_clock_reset(clock, graph);
        return;
    }

    /* Events may have changed the graph since the previous clock tick. */
    if (clock->state != graph->current) {
        unsigned_state_graph_clock_reset(clock, graph);
    }

    if (graph->current == NULL || graph->current->duration_frames == 0u) {
        return;
    }

    if (clock->elapsed_frames < UINT16_MAX) {
        ++clock->elapsed_frames;
    }
    if (clock->elapsed_frames < graph->current->duration_frames) {
        return;
    }

    if (unsigned_state_graph_timeout(graph)) {
        unsigned_state_graph_clock_reset(clock, graph);
    }
}
