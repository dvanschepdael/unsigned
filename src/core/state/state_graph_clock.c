/**
 * @file state_graph_clock.c
 * @brief Implements optional temporal execution for state graphs.
 */

#include "core/state/state_graph_clock.h"

void unsigned_state_graph_clock_reset(UStateGraphClock *clock, const UStateGraph *graph) {
    clock->state = graph->current;
    clock->elapsed_frames = 0u;
}

void unsigned_state_graph_clock_tick(UStateGraphClock *clock, UStateGraph *graph) {
    const UStateGraphNode *active = graph->current;
    unsigned_state_graph_tick(graph);

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

    const UStateGraphNode *before_timeout = graph->current;
    unsigned_state_graph_timeout(graph);
    if (graph->current != before_timeout) {
        unsigned_state_graph_clock_reset(clock, graph);
    }
}
